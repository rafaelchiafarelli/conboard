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
  device: string // DEVICE.name
  kind: 'midi' | 'evdev'
  // midi
  b0?: number
  b1?: number
  b2?: number
  // evdev
  code?: string
  edge?: string
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
    return false
  })
}

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
