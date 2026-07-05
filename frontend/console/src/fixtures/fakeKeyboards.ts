// Fake keyboard (evdev) devices — NOT from real board files; hand-authored to exercise
// the evdev editing path (keyboard triggers, code/mode edges, cross-type outputs) until
// a real keyboard board exists. They follow the real board schema (see boards/Xbox360.json):
// DEVICE.type === the trigger type, and the edge lives in the trigger's `mode` field.

import type { Board } from '../model/rules'

export const FAKE_KEYBOARDS: Board[] = [
  {
    DEVICE: { timeout: 0, type: 'keyboard', name: 'Macro Pad K1', input: 'conboard Macro Pad K1', output: '' },
    header: {
      identifier: {
        tags: { ID_BUS: 'usb', ID_VENDOR_ID: '1d50', ID_MODEL_ID: '6161', ID_MODEL: 'conboard_macro_pad' },
        executable: { exec: '/conboard/LowLevel/KeyBoard/build/conKeyB' },
      },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'keyboard', code: 'KEY_F1', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'text', data: 'git status', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_F2', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'hotKey', data: 'lControl lShift letter_p', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_F3', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'text', data: 'docker compose up -d', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_A', mode: 'hold' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'letter_b', hold: 'hold' }] },
            { input: { type: 'keyboard', code: 'KEY_ESC', mode: 'press' }, output: [], change_mode: { enable: true, change_to: 1 } },
          ],
        },
        {
          id: 1,
          active: false,
          actions: [
            { input: { type: 'keyboard', code: 'KEY_ESC', mode: 'press' }, output: [], change_mode: { enable: true, change_to: 0 } },
            { input: { type: 'keyboard', code: 'KEY_SPACE', mode: 'press' }, output: [{ type: 'midi', b0: 144, b1: 60, b2: 100 }] },
            { input: { type: 'keyboard', code: 'KEY_ENTER', mode: 'release' }, output: [{ type: 'keyboard', keyType: 'text', data: 'echo done', hold: 'not_hold' }] },
          ],
        },
      ],
    },
  },
  {
    DEVICE: { timeout: 0, type: 'keyboard', name: 'TKL Tester K2', input: 'conboard TKL Tester K2', output: '' },
    header: {
      identifier: {
        tags: { ID_BUS: 'usb', ID_VENDOR_ID: '1d50', ID_MODEL_ID: '6162', ID_MODEL: 'conboard_tkl_tester' },
        executable: { exec: '/conboard/LowLevel/KeyBoard/build/conKeyB' },
      },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'keyboard', code: 'KEY_UP', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'uArrow', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_DOWN', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'dArrow', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_LEFTCTRL', mode: 'hold' }, output: [{ type: 'keyboard', keyType: 'text', data: 'sudo ', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_1', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'n_one', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_2', mode: 'release' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'n_two', hold: 'not_hold' }] },
            { input: { type: 'keyboard', code: 'KEY_TAB', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'hotKey', data: 'lAlt tab', hold: 'not_hold' }] },
          ],
        },
      ],
    },
  },
]
