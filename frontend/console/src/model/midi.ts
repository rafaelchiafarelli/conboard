// MIDI decode helpers — turn raw status/data bytes into human-readable terms so
// the editor can show "Note On · ch1  note 1 = 64" instead of "144 1 64".

const MESSAGE_NAMES: Record<number, string> = {
  0x80: 'Note Off',
  0x90: 'Note On',
  0xa0: 'Aftertouch',
  0xb0: 'Control Change',
  0xc0: 'Program Change',
  0xd0: 'Channel Pressure',
  0xe0: 'Pitch Bend',
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
