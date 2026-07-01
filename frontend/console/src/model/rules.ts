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

/** Symbolic evdev trigger — not on this branch yet, modeled so it slots in later. */
export interface EvdevTrigger {
  type: 'evdev'
  code: string // e.g. "BTN_A", "KEY_ENTER", "ABS_X", "REL_WHEEL"
  edge: 'press' | 'release' | 'hold' | 'hold_once' | 'higher' | 'lower' | 'spot'
  delay?: number
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
