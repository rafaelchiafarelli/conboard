// The contract with the harpia-generated backend REST API (backend/generated).
//
// Empirically confirmed against the running backend (see backend wireup):
//  - JSON field names are camelCase (header_actions -> headerActions, key_type ->
//    keyType, mode_id -> modeId, change_mode_* -> changeMode*, wheel_move ->
//    wheelMove, right_click -> rightClick).
//  - the primary key is "ID<HASH>" and is CALLER-ASSIGNED (set before create).
//  - enums serialize as their NAME (dt_joystick, ak_keyboard, kt_one_key, ...).
//  - zero/default-valued fields are OMITTED on read (protobuf3 JSON), so an absent
//    enum means its 0-value and an absent int means 0.
//  - every route is credential-gated: X-User: <entity>, X-Pswd: <HASH>.
//  - CODEGEN QUIRK (found 2026-08-14): the harpia domain spec's trigger.interval
//    field comes out on the wire as "erval", not "interval" -- confirmed against
//    backend/generated/proto/protofiles/trigger_*.proto (field 10 is literally
//    `int32 erval`). Sending "interval" 400s the whole board create (protobuf JSON
//    rejects unknown fields) with no detail in the response body or backend log --
//    silently broke every evdev trigger using mode "hold" (interval) end to end.
//    Backend/generated is a black box (regenerated wholesale, never hand-edited),
//    so this is worked around here rather than in harpia itself.
//
// The HASH is the md5 of backend/harpia/conboard.harpia; it changes if the domain
// changes. Keep it in sync with backend/generated (a single source of truth here).

export const HASH = '9f20d5d43738774941f9898b22cf2cf2'
export const ID_KEY = `ID${HASH}` as const

export type Entity = 'board' | 'mode' | 'rule' | 'trigger' | 'output_action'

// ---- enum <-> frontend-literal maps ----------------------------------------
// harpia name on the left, the model/rules.ts literal on the right.

export const DEVICE_TYPE = {
  dt_midi: 'midi', dt_joystick: 'joystick', dt_keyboard: 'keyboard', dt_mouse: 'mouse',
} as const
export const TRIGGER_KIND = { tk_midi: 'midi', tk_evdev: 'evdev' } as const
export const TRIGGER_EDGE = {
  edge_press: 'press', edge_release: 'release', edge_hold: 'hold',
  edge_hold_once: 'hold_once', edge_higher: 'higher', edge_lower: 'lower', edge_spot: 'spot',
} as const
export const ACTION_KIND = { ak_midi: 'midi', ak_keyboard: 'keyboard', ak_mouse: 'mouse' } as const
export const KEY_TYPE = { kt_text: 'text', kt_one_key: 'oneKey', kt_hot_key: 'hotKey' } as const
export const HOLD_MODE = { hm_not_hold: 'not_hold', hm_hold: 'hold', hm_hold_once: 'hold_once' } as const
export const MIDI_MODE = {
  mm_normal: 'normal', mm_trigger_higher: 'trigger_higher', mm_trigger_lower: 'trigger_lower',
  mm_spot: 'spot', mm_blink: 'blink',
} as const

// invert a {harpiaName: literal} map into {literal: harpiaName}
function invert<T extends Record<string, string>>(m: T): Record<string, keyof T & string> {
  const out: Record<string, string> = {}
  for (const k in m) out[m[k]] = k
  return out as Record<string, keyof T & string>
}
export const DEVICE_TYPE_R = invert(DEVICE_TYPE)
export const TRIGGER_KIND_R = invert(TRIGGER_KIND)
export const TRIGGER_EDGE_R = invert(TRIGGER_EDGE)
export const ACTION_KIND_R = invert(ACTION_KIND)
export const KEY_TYPE_R = invert(KEY_TYPE)
export const HOLD_MODE_R = invert(HOLD_MODE)
export const MIDI_MODE_R = invert(MIDI_MODE)

// ---- raw harpia record shapes (what the REST API sends/accepts) ------------
// All fields optional: zero-valued ones are omitted on the wire.
export interface HarpiaId { [ID_KEY]?: number }

export interface HTrigger extends HarpiaId {
  kind?: keyof typeof TRIGGER_KIND
  b0?: number; b1?: number; b2?: number
  midiMode?: keyof typeof MIDI_MODE
  code?: string
  edge?: keyof typeof TRIGGER_EDGE
  value?: number; erval?: number; delay?: number  // erval: harpia codegen mangled "interval" -> "erval" (see trigger.proto), not a typo
}
export interface HOutputAction extends HarpiaId {
  kind?: keyof typeof ACTION_KIND
  delay?: number
  b0?: number; b1?: number; b2?: number
  data?: string
  keyType?: keyof typeof KEY_TYPE
  hold?: keyof typeof HOLD_MODE
  dx?: string; dy?: string; wheelMove?: string
  gotox?: string; gotoy?: string; click?: string; rightClick?: string
}
export interface HRule extends HarpiaId {
  input?: HTrigger
  outputs?: HOutputAction[]
  changeModeEnable?: number
  changeModeTo?: number
}
export interface HMode extends HarpiaId {
  modeId?: number
  active?: number
  modeHeader?: HOutputAction[]
  rules?: HRule[]
}
export interface HBoard extends HarpiaId {
  name?: string
  type?: keyof typeof DEVICE_TYPE
  timeout?: number
  input?: string; output?: string
  exec?: string; port?: string
  generics?: Record<string, string>
  tags?: Record<string, string>
  headerActions?: HOutputAction[]
  modes?: HMode[]
}
