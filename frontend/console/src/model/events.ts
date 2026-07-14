// Live event layer for the dispatcher stream. The whole app consumes ONE websocket
// (the liveBus below) and fans parsed events out to the views. Frames are the
// dispatcher's `<uuid>,<action>` text plus the proposed `HB,<uuid>,<devname>`
// heartbeat/roster frame (INTERFACE.md O3/O5). No simulated/mock source exists.

/** One incoming device event — a raw trigger the dispatcher reported. */
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

/** Connection state of the live socket, for the monitor's status indicator. */
export type LiveStatus = 'connecting' | 'open' | 'closed'

/** Same-origin dispatcher websocket URL (nginx maps /websocket → dispatcher /ws). */
export function defaultWsUrl(): string {
  const env = (import.meta as { env?: Record<string, string> }).env?.VITE_CONBOARD_WS
  if (env) return env
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:'
  return `${proto}//${location.host}/websocket`
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
