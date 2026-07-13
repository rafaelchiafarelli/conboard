// MIDI decode helpers — turn raw status/data bytes into human-readable terms so
// the editor can show "Note On · ch1  note 1 = 64" instead of "144 1 64".

import type { MidiMode } from './rules'

export const MESSAGE_NAMES: Record<number, string> = {
  0x80: 'Note Off',
  0x90: 'Note On',
  0xa0: 'Aftertouch',
  0xb0: 'Control Change',
  0xc0: 'Program Change',
  0xd0: 'Channel Pressure',
  0xe0: 'Pitch Bend',
}

/** Status byte 0x80..0xe0 → true when the message is Control Change (b1 = CC number). */
export function isCC(b0: number): boolean {
  return (b0 & 0xf0) === 0xb0
}

/** Split a status byte into its message type (high nibble) and 1-based channel. */
export function splitStatus(b0: number): { type: number; channel: number } {
  return { type: b0 & 0xf0, channel: (b0 & 0x0f) + 1 }
}

/** Combine a message type (high nibble) and 1-based channel back into a status byte. */
export function makeStatus(type: number, channel: number): number {
  return (type & 0xf0) | ((channel - 1) & 0x0f)
}

const SHORT_NAMES: Record<number, string> = {
  0x80: 'OFF',
  0x90: 'ON',
  0xa0: 'AT',
  0xb0: 'CC',
  0xc0: 'PC',
  0xd0: 'CP',
  0xe0: 'PB',
}

export interface DecodedMidi {
  /** e.g. "Note On · ch1" */
  human: string
  /** e.g. "note 1 = 64" or "CC 31 = 65" */
  detail: string
  /** e.g. "ON", "CC" */
  short: string
  channel: number
}

// ---- MIDI operation modes (firmware midi_action_mode) ----------------------
// These are MIDI-specific match behaviors implemented in the conMIDI handler; the
// editor surfaces them so a rule can use more than exact-value matching. `b2Label`
// is how the third byte should be labeled in that mode (it changes meaning).

export interface MidiModeInfo {
  value: MidiMode
  /** short label for the picker */
  label: string
  /** one-line explanation of the match behavior */
  hint: string
  /** what the b2 field means in this mode */
  b2Label: string
  /** true when b2 is not used to match (spot) */
  b2Ignored?: boolean
}

export const MIDI_MODES: MidiModeInfo[] = [
  { value: 'normal', label: 'Normal (exact)', hint: 'Fires when status, data 1 and data 2 all match exactly.', b2Label: 'Velocity / value (data 2)' },
  { value: 'trigger_higher', label: 'Trigger higher', hint: 'Fires when the incoming value rises ABOVE the threshold below.', b2Label: 'Threshold (fire when above)' },
  { value: 'trigger_lower', label: 'Trigger lower', hint: 'Fires when the incoming value drops BELOW the threshold below.', b2Label: 'Threshold (fire when below)' },
  { value: 'spot', label: 'Spot (value carry)', hint: 'Matches on status + data 1 only; the live value is carried through to the outputs.', b2Label: 'Value (ignored on match)', b2Ignored: true },
  { value: 'blink', label: 'Blink', hint: 'Matched like Normal; pair with an LED on/off MIDI output for blink feedback.', b2Label: 'Velocity / value (data 2)' },
]

export function midiModeInfo(mode: MidiMode | undefined): MidiModeInfo {
  return MIDI_MODES.find((m) => m.value === (mode ?? 'normal')) ?? MIDI_MODES[0]
}

export function decodeMidi(b0: number, b1: number, b2: number): DecodedMidi {
  const type = b0 & 0xf0
  const channel = (b0 & 0x0f) + 1
  let name = MESSAGE_NAMES[type] ?? 'MIDI'
  // Note On with velocity 0 is conventionally a Note Off.
  if (type === 0x90 && b2 === 0) name = 'Note Off'
  const short = SHORT_NAMES[type] ?? 'MIDI'
  const param = type === 0xb0 ? `CC ${b1}` : `note ${b1}`
  return { human: `${name} · ch${channel}`, detail: `${param} = ${b2}`, short, channel }
}

// ---- keyboard token prettifier ---------------------------------------------

const KEY_LABELS: Record<string, string> = {
  lArrow: '←',
  rArrow: '→',
  uArrow: '↑',
  dArrow: '↓',
  lControl: 'Ctrl',
  rControl: 'Ctrl',
  lShift: 'Shift',
  lAlt: 'Alt',
  enter: '⏎',
  '{spor}': 'spot',
}

/** Render a key token or space-separated combo (e.g. "lControl letter_u") readably. */
export function prettyKey(token: string): string {
  return token
    .split(/\s+/)
    .filter(Boolean)
    .map((t) => (t.startsWith('letter_') ? t.slice('letter_'.length).toUpperCase() : KEY_LABELS[t] ?? t))
    .join(' + ')
}
