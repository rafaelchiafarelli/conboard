// Fixtures derived from the real board files (boards/Arduino_Micro.json,
// boards/Dj4Mix.json). Trimmed to a representative subset for UI development —
// the DJ-Tech mode 0 has ~130 passthrough rules in the real file; a handful here
// is enough to build and eyeball the editor. This stands in for the backend's
// data provider until the (harpia-generated) management API exists.

import type { Board } from '../model/rules'

const rafaKeys = [
  { type: 'keyboard', keyType: 'oneKey', data: 'letter_r', hold: 'not_hold' },
  { type: 'keyboard', keyType: 'oneKey', data: 'letter_a', hold: 'not_hold' },
  { type: 'keyboard', keyType: 'oneKey', data: 'letter_f', hold: 'not_hold' },
  { type: 'keyboard', keyType: 'oneKey', data: 'letter_a', hold: 'not_hold' },
] as const

export const BOARDS: Board[] = [
  {
    DEVICE: { timeout: 0, type: 'midi', name: 'Arduino Micro', input: 'Arduino Micro', output: 'Arduino Micro' },
    header: {
      identifier: { executable: { exec: '/conboard/LowLevel/MIDI/build/conMIDI', port: 'hw:1,0,0' } },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'midi', b0: 144, b1: 1, b2: 64 }, output: [{ type: 'keyboard', keyType: 'text', data: 'sudo systemctl restart ', hold: 'not_hold' }] },
            { input: { type: 'midi', b0: 144, b1: 2, b2: 64 }, output: [{ type: 'keyboard', keyType: 'text', data: 'sudo systemctl status ', hold: 'not_hold' }] },
            {
              input: { type: 'midi', b0: 144, b1: 3, b2: 64 },
              output: [
                { type: 'keyboard', keyType: 'text', data: 'sudo journalctl -u  -f', hold: 'not_hold' },
                { type: 'keyboard', keyType: 'oneKey', data: 'lArrow', hold: 'not_hold' },
                { type: 'keyboard', keyType: 'oneKey', data: 'lArrow', hold: 'not_hold' },
                { type: 'keyboard', keyType: 'oneKey', data: 'lArrow', hold: 'not_hold' },
              ],
            },
            { input: { type: 'midi', b0: 144, b1: 4, b2: 64 }, output: [{ type: 'keyboard', keyType: 'text', data: 'cd /conboard/assets', hold: 'not_hold' }] },
            {
              input: { type: 'midi', b0: 144, b1: 5, b2: 64 },
              output: [
                { type: 'keyboard', keyType: 'hotKey', data: '{spor} lControl letter_u', hold: 'not_hold' },
                { type: 'keyboard', keyType: 'text', data: 'sudo systemctl status ', hold: 'not_hold' },
              ],
            },
            { input: { type: 'midi', b0: 144, b1: 6, b2: 64 }, output: [{ type: 'keyboard', keyType: 'text', data: 'This is a messagem to you! ', hold: 'not_hold' }] },
          ],
        },
        {
          id: 1,
          active: false,
          actions: [
            { input: { type: 'midi', b0: 145, b1: 10, b2: 0 }, output: [{ type: 'midi', b0: 145, b1: 2, b2: 127 }, ...rafaKeys] },
            { input: { type: 'midi', b0: 145, b1: 9, b2: 127 }, output: [{ type: 'midi', b0: 145, b1: 2, b2: 127 }, ...rafaKeys] },
          ],
        },
      ],
    },
  },
  {
    DEVICE: { timeout: 0, type: 'midi', name: 'DJ-Tech 4-Mix', input: 'DJ-Tech 4-Mix', output: 'DJ-Tech 4-Mix' },
    header: {
      identifier: { executable: { exec: '/conboard/LowLevel/MIDI/build/conMIDI', port: 'hw:0,0,0' } },
      actions: [],
    },
    body: {
      modes: [
        {
          id: 0,
          active: true,
          actions: [
            { input: { type: 'midi', b0: 144, b1: 34, b2: 127 }, output: [{ type: 'midi', b0: 144, b1: 34, b2: 127 }] },
            { input: { type: 'midi', b0: 144, b1: 35, b2: 127 }, output: [{ type: 'midi', b0: 144, b1: 35, b2: 127 }] },
            { input: { type: 'midi', b0: 145, b1: 1, b2: 127 }, output: [{ type: 'midi', b0: 145, b1: 1, b2: 127 }] },
            { input: { type: 'midi', b0: 144, b1: 58, b2: 0 }, output: [{ type: 'midi', b0: 144, b1: 58, b2: 0 }] },
            { input: { type: 'midi', b0: 147, b1: 66, b2: 127 }, output: [{ type: 'midi', b0: 147, b1: 66, b2: 127 }] },
            { input: { type: 'midi', b0: 176, b1: 1, b2: 0 }, output: [{ type: 'midi', b0: 176, b1: 1, b2: 0 }] },
            { input: { type: 'midi', b0: 176, b1: 31, b2: 65 }, output: [{ type: 'midi', b0: 176, b1: 31, b2: 65 }] },
            { input: { type: 'midi', b0: 180, b1: 79, b2: 65 }, output: [{ type: 'midi', b0: 180, b1: 79, b2: 65 }] },
          ],
        },
        {
          id: 1,
          active: false,
          actions: [
            { input: { type: 'midi', b0: 144, b1: 58, b2: 127 }, output: [], change_mode: { enable: true, change_to: 0 } },
            { input: { type: 'midi', b0: 145, b1: 10, b2: 127 }, output: [{ type: 'midi', b0: 145, b1: 2, b2: 127 }, ...rafaKeys] },
            { input: { type: 'midi', b0: 145, b1: 9, b2: 127 }, output: [{ type: 'midi', b0: 145, b1: 2, b2: 127 }, ...rafaKeys] },
          ],
        },
      ],
    },
  },
]
