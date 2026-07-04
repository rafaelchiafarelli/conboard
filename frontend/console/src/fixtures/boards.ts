// Fixtures generated verbatim from the real board files (boards/Arduino_Micro.json,
// boards/Dj4Mix.json) — the on-device runtime source of truth. This is the COMPLETE
// configuration (DJ-Tech mode 0 carries all ~198 passthrough rules), not a trimmed
// subset. Regenerate from the source JSON rather than hand-editing. Stands in for the
// backend's data provider until the (harpia-generated) management API exists.
//
// Shape matches ../model/rules.ts one-to-one (that model was authored to mirror these
// files), so the JSON drops straight in as a typed Board[].

import type { Board } from '../model/rules'

export const BOARDS: Board[] = [
  {
    "DEVICE": {
      "timeout": 0,
      "type": "midi",
      "name": "Arduino Micro",
      "input": "Arduino Micro",
      "output": "Arduino Micro"
    },
    "header": {
      "identifier": {
        "generics": {
          "REALTEK_NIC_MODE": "1",
          "PWD": "/",
          "DEVPATH": "/devices/platform/soc/1c1b00.usb/usb3/3-1/3-1.1",
          "USEC_INITIALIZED": "79890646819",
          "SUBSYSTEM": "usb",
          "BUSNUM": "03",
          "DEVNAME": "/dev/bus/usb/03/035",
          "ACTION": "add",
          "_": "/usr/bin/env"
        },
        "tags": {
          "ID_MODEL": "Arduino_Micro",
          "ID_SERIAL": "Arduino_LLC_Arduino_Micro_MIDI",
          "ID_BUS": "usb",
          "TYPE": "0/0/0",
          "ID_MODEL_ID": "8037",
          "ID_VENDOR_ENC": "Arduino\\x20LLC",
          "ID_VENDOR_ID": "2341",
          "ID_USB_INTERFACES": ":020200:0a0000:010100:010300:",
          "ID_VENDOR_FROM_DATABASE": "Arduino SA",
          "ID_SERIAL_SHORT": "MIDI",
          "ID_MODEL_ENC": "Arduino\\x20Micro",
          "PRODUCT": "2341/8037/100",
          "DRIVER": "usb",
          "DEVTYPE": "usb_device",
          "ID_VENDOR": "Arduino_LLC",
          "MAJOR": "189"
        },
        "executable": {
          "exec": "/conboard/LowLevel/MIDI/build/conMIDI",
          "port": "hw:1,0,0"
        }
      },
      "actions": [
        {
          "type": "midi",
          "b0": 145,
          "b1": 1,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 2,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 3,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 3,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 4,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 5,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 6,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 7,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 9,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 10,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 11,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 12,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 13,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 14,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 15,
          "b2": 255,
          "delay": 50
        },
        {
          "type": "mouse",
          "dx": "+12",
          "dy": "-12",
          "wheel_move": "+12",
          "gotox": "23",
          "gotoy": "10",
          "click": "true",
          "right_click": "true",
          "delay": "2000"
        }
      ]
    },
    "body": {
      "modes": [
        {
          "id": 0,
          "active": true,
          "mode_header": {
            "actions": [
              {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 11,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 12,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 13,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 14,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 15,
                "b2": 255,
                "delay": 50
              }
            ]
          },
          "actions": [
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 1,
                "b2": 64
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "sudo systemctl restart ",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 2,
                "b2": 64,
                "delay": 50
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "sudo systemctl status ",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 3,
                "b2": 64,
                "delay": 50
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "sudo journalctl -u  -f",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "lArrow",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "lArrow",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "lArrow",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 4,
                "b2": 64,
                "delay": 50
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "cd /conboard/assets",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 64,
                "delay": 50
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "{spor} lControl letter_u",
                  "delay": 0,
                  "keyType": "hotKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "sudo systemctl status ",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 64,
                "delay": 50
              },
              "output": [
                {
                  "type": "keyboard",
                  "data": "This is a messagem to you! ",
                  "delay": 0,
                  "keyType": "text",
                  "hold": "not_hold"
                }
              ]
            }
          ]
        },
        {
          "id": 1,
          "active": false,
          "mode_header": {
            "actions": [
              {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 11,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 12,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 13,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 14,
                "b2": 255,
                "delay": 50
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 15,
                "b2": 255,
                "delay": 50
              }
            ]
          },
          "actions": [
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 0,
                "delay": 50
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                },
                {
                  "type": "keyboard",
                  "data": "letter_r",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_f",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 127,
                "delay": 50
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                },
                {
                  "type": "keyboard",
                  "data": "letter_r",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_f",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 9,
                "b2": 127,
                "delay": 50
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                },
                {
                  "type": "keyboard",
                  "data": "letter_r",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_f",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            }
          ]
        }
      ]
    }
  },
  {
    "DEVICE": {
      "timeout": 0,
      "type": "midi",
      "name": "DJ-Tech 4-Mix",
      "input": "DJ-Tech 4-Mix",
      "output": "DJ-Tech 4-Mix"
    },
    "header": {
      "identifier": {
        "generics": {},
        "tags": {
          "ID_MODEL": "DJ-Tech_4-Mix"
        },
        "executable": {
          "exec": "/conboard/LowLevel/MIDI/build/conMIDI",
          "port": "hw:0,0,0"
        }
      },
      "actions": [
        {
          "type": "midi",
          "b0": 145,
          "b1": 1,
          "b2": 127,
          "delay": 250
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 1,
          "b2": 0
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 2,
          "b2": 127,
          "delay": 250
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 2,
          "b2": 0
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 3,
          "b2": 127,
          "delay": 250
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 3,
          "b2": 0
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 4,
          "b2": 127,
          "delay": 250
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 4,
          "b2": 0
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 5,
          "b2": 127,
          "delay": 250
        },
        {
          "type": "midi",
          "b0": 145,
          "b1": 5,
          "b2": 0
        }
      ]
    },
    "body": {
      "modes": [
        {
          "id": 0,
          "active": true,
          "mode_header": {
            "actions": [
              {
                "type": "midi",
                "b0": 145,
                "b1": 1,
                "b2": 127,
                "delay": 250
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 1,
                "b2": 0
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 2,
                "b2": 127,
                "delay": 250
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 2,
                "b2": 0
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 3,
                "b2": 127,
                "delay": 250
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 3,
                "b2": 0
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 4,
                "b2": 127,
                "delay": 250
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 4,
                "b2": 0
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 5,
                "b2": 127,
                "delay": 250
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 5,
                "b2": 0
              }
            ]
          },
          "actions": [
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 34,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 34,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 35,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 35,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 36,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 36,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 37,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 37,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 1,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 1,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 37,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 37,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 38,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 38,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 39,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 39,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 40,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 40,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 41,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 41,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 42,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 42,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 43,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 43,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 44,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 44,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 45,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 45,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 58,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 58,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 59,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 59,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 59,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 59,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 58,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 58,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 2,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 66,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 66,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 2,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 2,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 66,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 66,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 3,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 3,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 67,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 67,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 3,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 3,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 67,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 67,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 4,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 4,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 68,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 68,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 4,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 4,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 68,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 68,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 69,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 69,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 69,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 69,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 70,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 70,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 70,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 70,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 71,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 71,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 71,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 71,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 72,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 72,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 72,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 72,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 9,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 9,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 73,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 73,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 9,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 9,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 73,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 73,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 10,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 74,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 74,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 10,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 10,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 74,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 74,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 11,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 11,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 75,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 75,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 11,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 11,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 75,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 75,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 12,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 12,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 76,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 76,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 12,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 12,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 76,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 76,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 14,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 14,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 14,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 14,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 61,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 61,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 60,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 60,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 60,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 60,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 61,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 61,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 2,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 2,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 66,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 66,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 2,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 2,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 66,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 66,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 3,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 3,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 67,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 67,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 3,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 3,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 67,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 67,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 4,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 4,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 68,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 68,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 4,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 4,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 68,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 68,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 69,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 69,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 69,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 69,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 70,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 70,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 70,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 70,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 71,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 71,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 71,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 71,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 72,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 72,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 72,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 72,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 9,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 9,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 73,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 73,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 9,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 9,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 73,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 73,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 10,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 10,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 74,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 74,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 10,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 10,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 74,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 74,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 11,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 11,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 75,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 75,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 11,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 11,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 75,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 75,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 12,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 12,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 76,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 76,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 12,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 12,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 76,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 76,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 14,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 14,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 14,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 14,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 1,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 1,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 2,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 2,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 3,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 3,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 4,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 4,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 6,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 6,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 7,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 7,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 8,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 8,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 5,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 5,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 26,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 26,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 27,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 27,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 31,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 31,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 31,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 31,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 31,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 31,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 31,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 31,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 176,
                "b1": 29,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 176,
                  "b1": 29,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 29,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 29,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 29,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 144,
                  "b1": 29,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 177,
                "b1": 79,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 177,
                  "b1": 79,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 177,
                "b1": 79,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 177,
                  "b1": 79,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 79,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 79,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 79,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 79,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 179,
                "b1": 79,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 179,
                  "b1": 79,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 179,
                "b1": 79,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 179,
                  "b1": 79,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 79,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 79,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 79,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 79,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 178,
                "b1": 79,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 178,
                  "b1": 79,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 178,
                "b1": 79,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 178,
                  "b1": 79,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 79,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 79,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 79,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 79,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 180,
                "b1": 79,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 180,
                  "b1": 79,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 180,
                "b1": 79,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 180,
                  "b1": 79,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 79,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 79,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 79,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 79,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 177,
                "b1": 15,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 177,
                  "b1": 15,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 177,
                "b1": 15,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 177,
                  "b1": 15,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 15,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 15,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 15,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 15,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 179,
                "b1": 15,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 179,
                  "b1": 15,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 179,
                "b1": 15,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 179,
                  "b1": 15,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 15,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 15,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 147,
                "b1": 15,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 147,
                  "b1": 15,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 178,
                "b1": 15,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 178,
                  "b1": 15,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 178,
                "b1": 15,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 178,
                  "b1": 15,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 15,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 15,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 146,
                "b1": 15,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 146,
                  "b1": 15,
                  "b2": 0
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 180,
                "b1": 15,
                "b2": 65
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 180,
                  "b1": 15,
                  "b2": 65
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 180,
                "b1": 15,
                "b2": 63
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 180,
                  "b1": 15,
                  "b2": 63
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 15,
                "b2": 127
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 15,
                  "b2": 127
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 148,
                "b1": 15,
                "b2": 0
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 148,
                  "b1": 15,
                  "b2": 0
                }
              ]
            }
          ]
        },
        {
          "id": 1,
          "active": false,
          "mode_header": {
            "actions": [
              {
                "type": "midi",
                "b0": 145,
                "b1": 2,
                "b2": 127
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 3,
                "b2": 127
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 4,
                "b2": 127
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 5,
                "b2": 127
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 6,
                "b2": 127
              },
              {
                "type": "midi",
                "b0": 145,
                "b1": 7,
                "b2": 127
              }
            ]
          },
          "actions": [
            {
              "input": {
                "type": "midi",
                "b0": 144,
                "b1": 58,
                "b2": 127
              },
              "output": [],
              "change_mode": {
                "enable": true,
                "change_to": 0
              }
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 10,
                "b2": 127,
                "delay": 50
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                },
                {
                  "type": "keyboard",
                  "data": "letter_r",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_f",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            },
            {
              "input": {
                "type": "midi",
                "b0": 145,
                "b1": 9,
                "b2": 127,
                "delay": 50
              },
              "output": [
                {
                  "type": "midi",
                  "b0": 145,
                  "b1": 2,
                  "b2": 127
                },
                {
                  "type": "keyboard",
                  "data": "letter_r",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_f",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                },
                {
                  "type": "keyboard",
                  "data": "letter_a",
                  "delay": 0,
                  "keyType": "oneKey",
                  "hold": "not_hold"
                }
              ]
            }
          ]
        }
      ]
    }
  }
]
