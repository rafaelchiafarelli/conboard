// Fake mouse (evdev) devices — NOT from real board files; hand-authored to exercise the
// mouse editing path. Mirrors docs/profile-templates/mouse.json: buttons are
// BTN_LEFT/BTN_RIGHT/BTN_MIDDLE/BTN_SIDE with press/release; motion/wheel are REL_* with
// higher/lower/spot + a `value` threshold. Outputs are type:keyboard (per the template,
// mouse/joystick HID outputs are currently no-ops on the device).

import type { Board } from '../model/rules'

export const FAKE_MICE: Board[] = [
  {
    DEVICE: { timeout: 0, type: 'mouse', name: 'Precision Mouse M1', input: 'conboard Precision Mouse M1', output: '' },
    header: {
      identifier: {
        tags: { ID_BUS: 'usb', ID_VENDOR_ID: '046d', ID_MODEL_ID: '4082', ID_MODEL: 'conboard_precision_mouse' },
        executable: { exec: '/conboard/LowLevel/Mouse/build/conMouse' },
      },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'mouse', code: 'BTN_LEFT', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'enter', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'BTN_RIGHT', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'hotKey', data: 'lControl letter_c', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'BTN_MIDDLE', mode: 'press' }, output: [], change_mode: { enable: true, change_to: 1 } },
            { input: { type: 'mouse', code: 'REL_WHEEL', mode: 'higher', value: 0 }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'uArrow', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'REL_WHEEL', mode: 'lower', value: 0 }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'dArrow', hold: 'not_hold' }] },
          ],
        },
        {
          id: 1,
          active: false,
          actions: [
            { input: { type: 'mouse', code: 'BTN_MIDDLE', mode: 'press' }, output: [], change_mode: { enable: true, change_to: 0 } },
            { input: { type: 'mouse', code: 'REL_X', mode: 'higher', value: 10 }, output: [{ type: 'keyboard', keyType: 'text', data: 'pan right ', hold: 'not_hold' }] },
          ],
        },
      ],
    },
  },
  {
    DEVICE: { timeout: 0, type: 'mouse', name: 'MMO Mouse M2', input: 'conboard MMO Mouse M2', output: '' },
    header: {
      identifier: {
        tags: { ID_BUS: 'usb', ID_VENDOR_ID: '1532', ID_MODEL_ID: '0067', ID_MODEL: 'conboard_mmo_mouse' },
        executable: { exec: '/conboard/LowLevel/Mouse/build/conMouse' },
      },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'mouse', code: 'BTN_SIDE', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'text', data: 'git commit -m ""', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'BTN_EXTRA', mode: 'press' }, output: [{ type: 'keyboard', keyType: 'hotKey', data: 'lAlt tab', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'BTN_LEFT', mode: 'hold', interval: 150 }, output: [{ type: 'keyboard', keyType: 'oneKey', data: 'space', hold: 'not_hold' }] },
            { input: { type: 'mouse', code: 'REL_HWHEEL', mode: 'spot', value: 0 }, output: [{ type: 'keyboard', keyType: 'text', data: 'hscroll ', hold: 'not_hold' }] },
          ],
        },
      ],
    },
  },
]
