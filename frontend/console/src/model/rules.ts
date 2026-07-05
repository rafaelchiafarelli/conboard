// The conboard rule model — a TypeScript mirror of the on-device board files
// (`boards/*.json`, e.g. Arduino_Micro.json, Dj4Mix.json), which are the runtime
// source of truth. The management DB / harpia layer is a *library* view over this
// same shape (see memory `conboard-rules-db-architecture`); this file stays the
// canonical editor model until the harpia-generated structs land, at which point
// it reconciles against them.
//
// Scope note: this branch's boards are MIDI-only. The trigger union below is
// deliberately open so the symbolic evdev triggers (BTN_*/KEY_*/ABS_* with
// press/release/hold/... from the feature/evdev-matcher branch) drop in without a
// remodel.

export type DeviceType = 'midi' | 'joystick' | 'keyboard' | 'mouse'

// ---- triggers (rule input) --------------------------------------------------

/** A raw MIDI message: status byte + two data bytes. */
export interface MidiTrigger {
  type: 'midi'
  b0: number // status byte (message type in the high nibble, channel in the low)
  b1: number // data 1 — note number or CC number
  b2: number // data 2 — velocity or CC value
  delay?: number
}

/** Evdev trigger edge (the firmware calls this field `mode` on the trigger). */
export type Edge = 'press' | 'release' | 'hold' | 'hold_once' | 'higher' | 'lower' | 'spot'

/** The non-MIDI device kinds, each driven by an evdev engine. */
export type EvdevKind = Exclude<DeviceType, 'midi'> // 'joystick' | 'keyboard' | 'mouse'

/**
 * Symbolic evdev trigger. Mirrors the real board files (e.g. boards/Xbox360.json):
 * the trigger `type` equals the device's DEVICE.type, and the edge is the `mode`
 * field — `{ "type": "joystick", "code": "BTN_SOUTH", "mode": "press" }`.
 */
export interface EvdevTrigger {
  type: EvdevKind
  code: string // e.g. "BTN_SOUTH", "KEY_ENTER", "ABS_X", "REL_WHEEL"
  mode: Edge
  value?: number // magnitude threshold for axis modes (higher/lower/spot on REL_*/ABS_*)
  interval?: number // repeat interval (ms) for mode 'hold'
  delay?: number // long-press delay (ms) for mode 'hold_once' (also a generic pre-delay)
}

export type Trigger = MidiTrigger | EvdevTrigger

// ---- outputs (rule actions) -------------------------------------------------

export interface MidiAction {
  type: 'midi'
  b0: number
  b1: number
  b2: number
  delay?: number
}

/** How the `data` field of a keyboard action is interpreted. */
export type KeyType = 'text' | 'oneKey' | 'hotKey'
export type HoldMode = 'not_hold' | 'hold' | 'hold_once'

export interface KeyboardAction {
  type: 'keyboard'
  data: string // literal text, a single key token (e.g. "letter_r", "lArrow"), or a combo
  keyType: KeyType
  hold: HoldMode
  delay?: number
}

export interface MouseAction {
  type: 'mouse'
  dx?: string
  dy?: string
  wheel_move?: string
  gotox?: string
  gotoy?: string
  click?: string // "true" when set
  right_click?: string
  delay?: string
}

export type OutputAction = MidiAction | KeyboardAction | MouseAction

// ---- rules, modes, devices --------------------------------------------------

/** Optional mode transition a rule can trigger instead of / alongside output. */
export interface ChangeMode {
  enable: boolean
  change_to: number
}

export interface Rule {
  input: Trigger
  output: OutputAction[]
  change_mode?: ChangeMode
}

export interface Mode {
  id: number
  active: boolean
  /** Actions fired on entering the mode (LED/handshake feedback on the device). */
  mode_header?: { actions: OutputAction[] }
  /** The rules: trigger → output mappings. */
  actions: Rule[]
}

export interface DeviceIdentity {
  generics?: Record<string, string>
  tags?: Record<string, string>
  executable?: { exec: string; port?: string }
}

export interface Board {
  DEVICE: {
    timeout: number
    type: DeviceType
    name: string
    input: string
    output: string
  }
  header: {
    identifier: DeviceIdentity
    actions: OutputAction[] // device-init/handshake actions
  }
  body: { modes: Mode[] }
}

// ---- validation -------------------------------------------------------------

/**
 * A rule's trigger type must equal its device's DEVICE.type — a MIDI device fires
 * only MIDI triggers, a keyboard only keyboard triggers, etc. (each board is driven
 * by one engine). Returns a human-readable problem per mismatch; empty when clean.
 * Run this when data arrives (fixtures today, the backend API later).
 */
export function validateBoards(boards: Board[]): string[] {
  const problems: string[] = []
  for (const b of boards) {
    for (const m of b.body.modes) {
      m.actions.forEach((r, i) => {
        if (r.input.type !== b.DEVICE.type) {
          problems.push(
            `${b.DEVICE.name} · mode ${m.id} · rule ${i + 1}: trigger type "${r.input.type}" ≠ device type "${b.DEVICE.type}"`,
          )
        }
      })
    }
  }
  return problems
}
