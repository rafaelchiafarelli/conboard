// HID vocabulary — the keyboard tokens and evdev codes the device firmware speaks,
// so the rule editor offers real, valid choices instead of free text.
//
// Key tokens mirror LowLevel/Common/src/keyNumber.cpp (the gperf token table used by
// keyboard outputs; keyType oneKey/hotKey). evdev codes mirror the libevdev symbolic
// names the evdev matcher accepts (feature/evdev-matcher; see boards/Xbox360.json).
// Both are curated to the practical set; a "custom" escape hatch in the editor keeps
// any other firmware token usable.

// ---- keyboard tokens -------------------------------------------------------

export const MODIFIER_TOKENS = ['lControl', 'rControl', 'lShift', 'rShift', 'lAlt', 'rAlt', 'lGUI', 'rGUI'] as const
const MOD_SET = new Set<string>(MODIFIER_TOKENS)

const NUMBER_WORDS = ['zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine']
const WORD_TO_DIGIT: Record<string, string> = Object.fromEntries(NUMBER_WORDS.map((w, i) => [w, String(i)]))

const letters = 'abcdefghijklmnopqrstuvwxyz'.split('').map((c) => `letter_${c}`)
const numbers = NUMBER_WORDS.map((w) => `n_${w}`)
const fkeys = Array.from({ length: 24 }, (_, i) => `f${i + 1}`)
const keypad = ['kZero', 'kOne', 'kTwo', 'kThree', 'kFour', 'kFive', 'kSix', 'kSeven', 'kEight', 'kNine', 'kPlus', 'kMinus', 'kMultiply', 'kForwardSlash', 'kEnter', 'kDot']

export interface TokenGroup {
  label: string
  tokens: string[]
}

export const KEY_TOKEN_GROUPS: TokenGroup[] = [
  { label: 'Modifiers', tokens: [...MODIFIER_TOKENS] },
  { label: 'Letters', tokens: letters },
  { label: 'Numbers', tokens: numbers },
  { label: 'Navigation', tokens: ['uArrow', 'dArrow', 'lArrow', 'rArrow', 'home', 'end', 'pageUp', 'pageDown', 'insert', 'deleteKey'] },
  { label: 'Editing & control', tokens: ['escape', 'enter', 'tab', 'space', 'capslock', 'printScreen', 'pause', 'menu', 'application', 'numLock', 'scrollLock'] },
  { label: 'Punctuation', tokens: ['minus', 'equal', 'comma', 'dot', 'forwardSlash', 'backSlash', 'singleCuotes', 'twoDots', 'Tilde', 'oBracket', 'cBracket'] },
  { label: 'Function', tokens: fkeys },
  { label: 'Keypad', tokens: keypad },
  { label: 'Media & system', tokens: ['vol_up', 'vol_down', 'mute', 'power', 'help', 'undo', 'cut', 'copy', 'past', 'find', 'again', 'stop', 'select', 'execute'] },
]

const KNOWN_TOKENS = new Set<string>(KEY_TOKEN_GROUPS.flatMap((g) => g.tokens))
export const isKnownToken = (t: string) => KNOWN_TOKENS.has(t)
export const isModifier = (t: string) => MOD_SET.has(t)

const KEY_LABELS: Record<string, string> = {
  lArrow: '←', rArrow: '→', uArrow: '↑', dArrow: '↓',
  lControl: 'Ctrl (L)', rControl: 'Ctrl (R)', lShift: 'Shift (L)', rShift: 'Shift (R)',
  lAlt: 'Alt (L)', rAlt: 'Alt (R)', lGUI: 'Super (L)', rGUI: 'Super (R)',
  escape: 'Esc', enter: 'Enter', tab: 'Tab', space: 'Space', capslock: 'Caps Lock',
  deleteKey: 'Delete', insert: 'Insert', home: 'Home', end: 'End', pageUp: 'Page Up', pageDown: 'Page Down',
  printScreen: 'Print Screen', pause: 'Pause', menu: 'Menu', application: 'Application',
  numLock: 'Num Lock', scrollLock: 'Scroll Lock',
  minus: '-', equal: '=', comma: ',', dot: '.', forwardSlash: '/', backSlash: '\\',
  singleCuotes: "'", twoDots: ':', Tilde: '~', oBracket: '[', cBracket: ']',
  kPlus: 'KP +', kMinus: 'KP −', kMultiply: 'KP *', kForwardSlash: 'KP /', kEnter: 'KP Enter', kDot: 'KP .',
  vol_up: 'Vol +', vol_down: 'Vol −', mute: 'Mute', power: 'Power', help: 'Help', undo: 'Undo',
  cut: 'Cut', copy: 'Copy', past: 'Paste', find: 'Find', again: 'Again', stop: 'Stop', select: 'Select', execute: 'Execute',
}

/** Human label for a single key token (e.g. "letter_r" → "R", "lControl" → "Ctrl (L)"). */
export function keyLabel(token: string): string {
  if (token.startsWith('letter_')) return token.slice(7).toUpperCase()
  if (token.startsWith('n_')) return WORD_TO_DIGIT[token.slice(2)] ?? token
  if (/^f\d+$/.test(token)) return token.toUpperCase()
  if (/^k(Zero|One|Two|Three|Four|Five|Six|Seven|Eight|Nine)$/.test(token))
    return `KP ${WORD_TO_DIGIT[token.slice(1).toLowerCase()] ?? token.slice(1)}`
  return KEY_LABELS[token] ?? token
}

/** Render a whole token string / combo readably (e.g. "lControl letter_u" → "Ctrl (L) + U"). */
export function comboLabel(data: string): string {
  return data
    .split(/\s+/)
    .filter(Boolean)
    .map((t) => keyLabel(t))
    .join(' + ')
}

/** Split a hotKey token string into its modifiers and the remaining (base) tokens. */
export function splitCombo(data: string): { mods: string[]; base: string } {
  const parts = data.split(/\s+/).filter(Boolean)
  return {
    mods: parts.filter((p) => MOD_SET.has(p)),
    base: parts.filter((p) => !MOD_SET.has(p)).join(' '),
  }
}

/** Compose a hotKey token string from ordered modifiers plus a base token string. */
export function joinCombo(mods: string[], base: string): string {
  return [...MODIFIER_TOKENS.filter((m) => mods.includes(m)), base.trim()].filter(Boolean).join(' ')
}

// ---- evdev codes -----------------------------------------------------------

export interface CodeGroup {
  label: string
  codes: string[]
}

export const EVDEV_CODE_GROUPS: CodeGroup[] = [
  { label: 'Gamepad buttons', codes: ['BTN_SOUTH', 'BTN_EAST', 'BTN_WEST', 'BTN_NORTH', 'BTN_TL', 'BTN_TR', 'BTN_TL2', 'BTN_TR2', 'BTN_SELECT', 'BTN_START', 'BTN_MODE', 'BTN_THUMBL', 'BTN_THUMBR', 'BTN_DPAD_UP', 'BTN_DPAD_DOWN', 'BTN_DPAD_LEFT', 'BTN_DPAD_RIGHT'] },
  { label: 'Joystick buttons', codes: ['BTN_TRIGGER', 'BTN_THUMB', 'BTN_THUMB2', 'BTN_TOP', 'BTN_TOP2', 'BTN_PINKIE', 'BTN_BASE', 'BTN_BASE2'] },
  { label: 'Mouse buttons', codes: ['BTN_LEFT', 'BTN_RIGHT', 'BTN_MIDDLE', 'BTN_SIDE', 'BTN_EXTRA'] },
  { label: 'Absolute axes', codes: ['ABS_X', 'ABS_Y', 'ABS_Z', 'ABS_RX', 'ABS_RY', 'ABS_RZ', 'ABS_HAT0X', 'ABS_HAT0Y'] },
  { label: 'Relative axes', codes: ['REL_X', 'REL_Y', 'REL_WHEEL', 'REL_HWHEEL'] },
  { label: 'Keyboard keys', codes: ['KEY_ESC', 'KEY_ENTER', 'KEY_SPACE', 'KEY_TAB', 'KEY_UP', 'KEY_DOWN', 'KEY_LEFT', 'KEY_RIGHT', 'KEY_LEFTCTRL', 'KEY_LEFTSHIFT', 'KEY_LEFTALT', 'KEY_A', 'KEY_W', 'KEY_S', 'KEY_D'] },
]

const KNOWN_CODES = new Set<string>(EVDEV_CODE_GROUPS.flatMap((g) => g.codes))
export const isKnownCode = (c: string) => KNOWN_CODES.has(c)

const GAMEPAD_LABELS: Record<string, string> = {
  BTN_SOUTH: 'A (South)', BTN_EAST: 'B (East)', BTN_WEST: 'X (West)', BTN_NORTH: 'Y (North)',
  BTN_TL: 'Left bumper', BTN_TR: 'Right bumper', BTN_TL2: 'Left trigger', BTN_TR2: 'Right trigger',
  BTN_SELECT: 'Select', BTN_START: 'Start', BTN_MODE: 'Mode', BTN_THUMBL: 'Left stick', BTN_THUMBR: 'Right stick',
  BTN_DPAD_UP: 'D-pad ↑', BTN_DPAD_DOWN: 'D-pad ↓', BTN_DPAD_LEFT: 'D-pad ←', BTN_DPAD_RIGHT: 'D-pad →',
  BTN_LEFT: 'Left click', BTN_RIGHT: 'Right click', BTN_MIDDLE: 'Middle click', BTN_SIDE: 'Side', BTN_EXTRA: 'Extra',
}

/** Human label for an evdev code (e.g. "BTN_SOUTH" → "A (South)"). */
export function codeLabel(code: string): string {
  if (GAMEPAD_LABELS[code]) return `${code} · ${GAMEPAD_LABELS[code]}`
  return code
}

/** True for continuous axes (ABS_ or REL_ codes), which use magnitude edges, not press/release. */
export const isAxis = (code: string) => code.startsWith('ABS_') || code.startsWith('REL_')

export type Edge = 'press' | 'release' | 'hold' | 'hold_once' | 'higher' | 'lower' | 'spot'

/** The edges that make sense for a given evdev code: axes get magnitude edges, buttons get press edges. */
export function edgesFor(code: string): Edge[] {
  return isAxis(code) ? ['higher', 'lower', 'spot'] : ['press', 'release', 'hold', 'hold_once']
}
