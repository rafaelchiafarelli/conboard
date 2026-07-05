// The device list the UI runs against: the real boards (generated from boards/*.json)
// plus hand-authored fake evdev devices for testing UI paths the real fixtures don't
// cover yet. This is the single import point for device data (swap for the backend
// data provider later); boards.ts stays a verbatim mirror of the real JSON.

import type { Board } from '../model/rules'
import { BOARDS as REAL_BOARDS } from './boards'
import { FAKE_KEYBOARDS } from './fakeKeyboards'

export const BOARDS: Board[] = [...REAL_BOARDS, ...FAKE_KEYBOARDS]
