// Synthetic 1:1 keyboard rule generation (docs/next-sessions/04-synthetic-1to1-rules.md):
// every KEY_* on a standard keyboard mapped to a rule that presses the identically-named
// output key. Both sides of the pair must resolve on the device:
//  - the input KEY_* name via LowLevel/Common/src/evMatch.cpp's kSymbols
//  - the output token via LowLevel/Common/src/keyNumber.cpp's oneKeySet
// (LowLevel/Common/include/keyNumbers.hash). Extend both tables together if this grows.
import type { EvdevTrigger, KeyboardAction, Rule } from './rules'

export const ONE_TO_ONE_KEYBOARD_MAP: readonly (readonly [string, string])[] = [
  // letters
  ['KEY_A', 'letter_a'], ['KEY_B', 'letter_b'], ['KEY_C', 'letter_c'], ['KEY_D', 'letter_d'],
  ['KEY_E', 'letter_e'], ['KEY_F', 'letter_f'], ['KEY_G', 'letter_g'], ['KEY_H', 'letter_h'],
  ['KEY_I', 'letter_i'], ['KEY_J', 'letter_j'], ['KEY_K', 'letter_k'], ['KEY_L', 'letter_l'],
  ['KEY_M', 'letter_m'], ['KEY_N', 'letter_n'], ['KEY_O', 'letter_o'], ['KEY_P', 'letter_p'],
  ['KEY_Q', 'letter_q'], ['KEY_R', 'letter_r'], ['KEY_S', 'letter_s'], ['KEY_T', 'letter_t'],
  ['KEY_U', 'letter_u'], ['KEY_V', 'letter_v'], ['KEY_W', 'letter_w'], ['KEY_X', 'letter_x'],
  ['KEY_Y', 'letter_y'], ['KEY_Z', 'letter_z'],
  // digits
  ['KEY_1', 'n_one'], ['KEY_2', 'n_two'], ['KEY_3', 'n_three'], ['KEY_4', 'n_four'],
  ['KEY_5', 'n_five'], ['KEY_6', 'n_six'], ['KEY_7', 'n_seven'], ['KEY_8', 'n_eight'],
  ['KEY_9', 'n_nine'], ['KEY_0', 'n_zero'],
  // function keys
  ['KEY_F1', 'f1'], ['KEY_F2', 'f2'], ['KEY_F3', 'f3'], ['KEY_F4', 'f4'],
  ['KEY_F5', 'f5'], ['KEY_F6', 'f6'], ['KEY_F7', 'f7'], ['KEY_F8', 'f8'],
  ['KEY_F9', 'f9'], ['KEY_F10', 'f10'], ['KEY_F11', 'f11'], ['KEY_F12', 'f12'],
  // modifiers
  ['KEY_LEFTCTRL', 'lControl'], ['KEY_RIGHTCTRL', 'rControl'],
  ['KEY_LEFTSHIFT', 'lShift'], ['KEY_RIGHTSHIFT', 'rShift'],
  ['KEY_LEFTALT', 'lAlt'], ['KEY_RIGHTALT', 'rAlt'],
  ['KEY_LEFTMETA', 'lGUI'], ['KEY_RIGHTMETA', 'rGUI'],
  // punctuation
  ['KEY_MINUS', 'minus'], ['KEY_EQUAL', 'equal'],
  ['KEY_LEFTBRACE', 'oBracket'], ['KEY_RIGHTBRACE', 'cBracket'],
  ['KEY_BACKSLASH', 'backSlash'], ['KEY_SEMICOLON', 'twoDots'],
  ['KEY_APOSTROPHE', 'singleCuotes'], ['KEY_GRAVE', 'acuteAcent'],
  ['KEY_COMMA', 'comma'], ['KEY_DOT', 'dot'], ['KEY_SLASH', 'forwardSlash'],
  // whitespace / control
  ['KEY_ESC', 'escape'], ['KEY_TAB', 'tab'], ['KEY_CAPSLOCK', 'capslock'],
  ['KEY_ENTER', 'enter'], ['KEY_BACKSPACE', 'deleteKey'], ['KEY_SPACE', 'space'],
  // navigation
  ['KEY_INSERT', 'insert'], ['KEY_DELETE', 'kDeleteKey'],
  ['KEY_HOME', 'home'], ['KEY_END', 'end'],
  ['KEY_PAGEUP', 'pageUp'], ['KEY_PAGEDOWN', 'pageDown'],
  ['KEY_UP', 'uArrow'], ['KEY_DOWN', 'dArrow'], ['KEY_LEFT', 'lArrow'], ['KEY_RIGHT', 'rArrow'],
  // locks / system
  ['KEY_NUMLOCK', 'numLock'], ['KEY_SCROLLLOCK', 'scrollLock'],
  ['KEY_SYSRQ', 'printScreen'], ['KEY_PAUSE', 'pause'],
  // numeric keypad
  ['KEY_KP0', 'kZero'], ['KEY_KP1', 'kOne'], ['KEY_KP2', 'kTwo'], ['KEY_KP3', 'kThree'],
  ['KEY_KP4', 'kFour'], ['KEY_KP5', 'kFive'], ['KEY_KP6', 'kSix'], ['KEY_KP7', 'kSeven'],
  ['KEY_KP8', 'kEight'], ['KEY_KP9', 'kNine'], ['KEY_KPDOT', 'kDot'],
  ['KEY_KPSLASH', 'kForwardSlash'], ['KEY_KPASTERISK', 'kMultiply'],
  ['KEY_KPMINUS', 'kMinus'], ['KEY_KPPLUS', 'kPlus'], ['KEY_KPENTER', 'kEnter'],
  ['KEY_KPEQUAL', 'kEqual0'],
  // menu / app key
  ['KEY_COMPOSE', 'application'],
] as const

/** Builds one press-triggered rule per entry in ONE_TO_ONE_KEYBOARD_MAP. */
export function buildOneToOneKeyboardRules(): Rule[] {
  return ONE_TO_ONE_KEYBOARD_MAP.map(([code, data]) => {
    const input: EvdevTrigger = { type: 'keyboard', code, mode: 'press' }
    const output: KeyboardAction = { type: 'keyboard', data, keyType: 'oneKey', hold: 'not_hold' }
    return { input, output: [output] }
  })
}
