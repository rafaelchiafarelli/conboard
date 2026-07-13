// Live-monitor event layer. The console shows a feed of incoming device events and
// highlights which rule (if any) in the device's live mode they'd fire.
//
// There's no backend yet, so events come from SimulatedEventSource. The EventSource
// interface is the swap seam: a WebSocketEventSource that consumes the dispatcher's
// realtime stream (INTERFACE.md) implements the same two methods and the Monitor UI
// is unchanged.

import type { Board, Rule, MidiTrigger, EvdevTrigger } from './rules'

/** One incoming device event — a raw trigger the device reported. */
export interface DeviceEvent {
  id: number
  ts: number // epoch ms
  device: string // DEVICE.name (simulated) or the sender uuid (live)
  kind: 'midi' | 'evdev' | 'raw'
  // midi
  b0?: number
  b1?: number
  b2?: number
  // evdev
  code?: string
  edge?: string
  // live dispatcher stream (kind === 'raw'): opaque per INTERFACE.md O3
  uuid?: string
  raw?: string
  // resolved from the HB roster frame (INTERFACE.md O5) when the dispatcher emits it;
  // lets a view filter the single stream to one configured device.
  devname?: string
}

export interface EventSource {
  start(onEvent: (e: DeviceEvent) => void): void
  stop(): void
}

/**
 * Find the rule in a device's live mode whose trigger matches an incoming event.
 * MIDI matches on status+data1 (b0/b1); b2 is velocity/value and varies per hit.
 */
export function matchEvent(board: Board, e: DeviceEvent): Rule | undefined {
  const mode = board.body.modes.find((m) => m.active)
  if (!mode) return undefined
  return mode.actions.find((r) => {
    const t = r.input
    if (e.kind === 'midi' && t.type === 'midi') return t.b0 === e.b0 && t.b1 === e.b1
    if (e.kind === 'evdev' && t.type !== 'midi') return t.code === e.code && t.mode === e.edge
    return false // 'raw' live frames are opaque (O3) — no structured match
  })
}

/** Connection state of a live event source, for the monitor's status indicator. */
export type LiveStatus = 'connecting' | 'open' | 'closed'

/** Same-origin dispatcher websocket URL (nginx maps /websocket → dispatcher /ws). */
export function defaultWsUrl(): string {
  const env = (import.meta as { env?: Record<string, string> }).env?.VITE_CONBOARD_WS
  if (env) return env
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:'
  return `${proto}//${location.host}/websocket`
}

/**
 * Real event stream from the dispatcher's live user-action websocket. Frames are
 * `<uuid>,<action>` text (a CSV `UUID,UserAction` header arrives first on connect);
 * the action is an opaque device string (INTERFACE.md O3), so events are emitted as
 * kind 'raw' and shown verbatim rather than matched against rules. Auto-reconnects.
 */
export class WebSocketEventSource implements EventSource {
  private ws: WebSocket | null = null
  private seq = 0
  private stopped = false
  private retry: ReturnType<typeof setTimeout> | null = null

  constructor(private url: string, private onStatus?: (s: LiveStatus) => void) {}

  start(onEvent: (e: DeviceEvent) => void): void {
    this.open(onEvent)
  }

  private open(onEvent: (e: DeviceEvent) => void): void {
    if (this.stopped) return
    this.onStatus?.('connecting')
    let ws: WebSocket
    try {
      ws = new WebSocket(this.url)
    } catch {
      this.scheduleRetry(onEvent)
      return
    }
    this.ws = ws
    ws.onopen = () => this.onStatus?.('open')
    ws.onmessage = (ev) => {
      if (typeof ev.data !== 'string') return
      for (const line of ev.data.split(/\r?\n/)) {
        const row = line.trim()
        if (!row || row.startsWith('UUID,')) continue // skip the CSV header row
        const comma = row.indexOf(',')
        const uuid = comma >= 0 ? row.slice(0, comma) : ''
        const action = comma >= 0 ? row.slice(comma + 1) : row
        onEvent({ id: ++this.seq, ts: Date.now(), device: uuid || 'device', kind: 'raw', uuid, raw: action })
      }
    }
    ws.onclose = () => {
      this.onStatus?.('closed')
      this.scheduleRetry(onEvent)
    }
    ws.onerror = () => ws.close()
  }

  private scheduleRetry(onEvent: (e: DeviceEvent) => void): void {
    if (this.stopped || this.retry) return
    this.retry = setTimeout(() => {
      this.retry = null
      this.open(onEvent)
    }, 3000)
  }

  stop(): void {
    this.stopped = true
    if (this.retry) clearTimeout(this.retry)
    this.retry = null
    if (this.ws) {
      this.ws.onclose = null // don't retry on an intentional close
      this.ws.close()
    }
    this.ws = null
  }
}

/**
 * Shared live bus — ONE websocket for the whole app.
 *
 * The dispatcher feed is a SINGLE stream keyed by sender uuid (INTERFACE.md §3), so
 * device-centric views must filter it rather than expect a per-device socket. The bus
 * keeps a uuid→devname roster and per-device liveness, both fed by the proposed
 * `HB,<uuid>,<devname>` heartbeat frame (O5), and fans parsed action events out to
 * subscribers. The socket is ref-counted: it opens on the first subscriber and closes
 * when the last leaves. Fallback is graceful — with no HB frames the roster/liveness
 * stay empty, events carry only the uuid, and per-device views show all senders.
 */
export interface LiveHandlers {
  onEvent?: (e: DeviceEvent) => void
  onStatus?: (s: LiveStatus) => void
  onTick?: () => void // roster/liveness changed — re-render LEDs
}

class LiveBus {
  private ws: WebSocket | null = null
  private retry: ReturnType<typeof setTimeout> | null = null
  private seq = 0
  private refs = 0
  private status: LiveStatus = 'connecting'
  private subs = new Set<LiveHandlers>()
  readonly roster = new Map<string, string>()   // uuid -> devname
  readonly liveness = new Map<string, number>() // devname -> last heartbeat (epoch ms)

  subscribe(h: LiveHandlers): () => void {
    this.subs.add(h)
    h.onStatus?.(this.status)
    this.refs++
    this.open()
    return () => {
      this.subs.delete(h)
      if (--this.refs <= 0) this.close()
    }
  }

  /** True if an HB for this devname arrived within `withinMs` (heartbeat liveness). */
  isLive(devname: string, withinMs = 4000): boolean {
    const t = this.liveness.get(devname)
    return t != null && Date.now() - t < withinMs
  }

  private setStatus(s: LiveStatus) { this.status = s; this.subs.forEach((h) => h.onStatus?.(s)) }

  private open() {
    if (this.ws) return
    this.setStatus('connecting')
    let ws: WebSocket
    try { ws = new WebSocket(defaultWsUrl()) } catch { this.scheduleRetry(); return }
    this.ws = ws
    ws.onopen = () => this.setStatus('open')
    ws.onmessage = (ev) => { if (typeof ev.data === 'string') this.onData(ev.data) }
    ws.onclose = () => { this.ws = null; this.setStatus('closed'); this.scheduleRetry() }
    ws.onerror = () => ws.close()
  }

  private scheduleRetry() {
    if (this.refs <= 0 || this.retry) return
    this.retry = setTimeout(() => { this.retry = null; if (this.refs > 0) this.open() }, 3000)
  }

  private close() {
    if (this.retry) { clearTimeout(this.retry); this.retry = null }
    if (this.ws) { this.ws.onclose = null; this.ws.close(); this.ws = null }
  }

  private onData(data: string) {
    let rosterChanged = false
    for (const line of data.split(/\r?\n/)) {
      const row = line.trim()
      if (!row || row.startsWith('UUID,')) continue // skip CSV header
      // Heartbeat/roster frame (O5): HB,<uuid>,<devname>
      if (row.startsWith('HB,')) {
        const p = row.split(',')
        const uuid = p[1] ?? ''
        const devname = p.slice(2).join(',')
        if (uuid && devname) { this.roster.set(uuid, devname); this.liveness.set(devname, Date.now()); rosterChanged = true }
        continue
      }
      // Action frame (unchanged): <uuid>,<action>; action may itself contain commas.
      const c = row.indexOf(',')
      const uuid = c >= 0 ? row.slice(0, c) : ''
      const action = c >= 0 ? row.slice(c + 1) : row
      const devname = this.roster.get(uuid)
      const e: DeviceEvent = { id: ++this.seq, ts: Date.now(), device: devname || uuid || 'device', kind: 'raw', uuid, raw: action }
      if (devname) e.devname = devname
      this.subs.forEach((h) => h.onEvent?.(e))
    }
    if (rosterChanged) this.subs.forEach((h) => h.onTick?.())
  }
}

/** Process-wide shared live bus (one dispatcher socket for all views). */
export const liveBus = new LiveBus()

/**
 * Stand-in event stream: emits at a jittered cadence, mostly replaying real mapped
 * triggers from each device's live mode (so matches light up) with some unmapped
 * noise mixed in. Replace with a WebSocketEventSource once the backend exists.
 */
export class SimulatedEventSource implements EventSource {
  private timer: ReturnType<typeof setTimeout> | null = null
  private seq = 0

  constructor(private boards: Board[]) {}

  start(onEvent: (e: DeviceEvent) => void): void {
    const tick = () => {
      onEvent(this.next())
      this.timer = setTimeout(tick, 450 + Math.random() * 1150)
    }
    this.timer = setTimeout(tick, 350)
  }

  stop(): void {
    if (this.timer) clearTimeout(this.timer)
    this.timer = null
  }

  private next(): DeviceEvent {
    const board = this.boards[Math.floor(Math.random() * this.boards.length)]
    const mode = board.body.modes.find((m) => m.active) ?? board.body.modes[0]
    const base = { id: ++this.seq, ts: Date.now(), device: board.DEVICE.name }

    if (board.DEVICE.type === 'midi') {
      const midiRules = mode ? mode.actions.filter((r) => r.input.type === 'midi') : []
      // 70%: replay a mapped trigger so the feed shows real matches.
      if (midiRules.length && Math.random() < 0.7) {
        const t = midiRules[Math.floor(Math.random() * midiRules.length)].input as MidiTrigger
        const cc = (t.b0 & 0xf0) === 0xb0
        return { ...base, kind: 'midi', b0: t.b0, b1: t.b1, b2: cc ? Math.floor(Math.random() * 128) : t.b2 || 100 }
      }
      // 30%: unmapped note on channel 1.
      return { ...base, kind: 'midi', b0: 144, b1: Math.floor(Math.random() * 128), b2: Math.floor(Math.random() * 127) + 1 }
    }

    // evdev device (joystick/keyboard/mouse): replay a mapped code, or unmapped noise.
    const evRules = mode ? mode.actions.filter((r) => r.input.type !== 'midi') : []
    if (evRules.length && Math.random() < 0.75) {
      const t = evRules[Math.floor(Math.random() * evRules.length)].input as EvdevTrigger
      return { ...base, kind: 'evdev', code: t.code, edge: t.mode }
    }
    const noise = ['KEY_Q', 'KEY_Z', 'KEY_M', 'KEY_P', 'KEY_TAB']
    return { ...base, kind: 'evdev', code: noise[Math.floor(Math.random() * noise.length)], edge: 'press' }
  }
}
