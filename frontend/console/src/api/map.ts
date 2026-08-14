// Translate between the harpia REST records (api/harpia.ts) and the frontend model
// (model/rules.ts). harpia flattens the model's unions onto a "kind" discriminant and
// normalises the aggregate across tables; here we fold it back into the nested Board
// the UI edits, and unfold it for writes.
import type {
  Board, Mode, Rule, Trigger, OutputAction, DeviceType, EvdevKind,
  MidiAction, KeyboardAction, MouseAction, Edge, KeyType, HoldMode, MidiMode,
} from '../model/rules'
import {
  ID_KEY,
  DEVICE_TYPE, DEVICE_TYPE_R, TRIGGER_EDGE, TRIGGER_EDGE_R,
  ACTION_KIND, ACTION_KIND_R, KEY_TYPE, KEY_TYPE_R, HOLD_MODE, HOLD_MODE_R,
  MIDI_MODE, MIDI_MODE_R,
  type HBoard, type HMode, type HRule, type HTrigger, type HOutputAction,
} from './harpia'

// ============================ harpia -> frontend ============================

function actionFromH(h: HOutputAction): OutputAction {
  const kind = ACTION_KIND[h.kind ?? 'ak_midi']  // omitted => 0-value ak_midi
  const delay = h.delay
  if (kind === 'keyboard') {
    const a: KeyboardAction = {
      type: 'keyboard',
      data: h.data ?? '',
      keyType: (KEY_TYPE[h.keyType ?? 'kt_text']) as KeyType,
      hold: (HOLD_MODE[h.hold ?? 'hm_not_hold']) as HoldMode,
    }
    if (delay != null) a.delay = delay
    return a
  }
  if (kind === 'mouse') {
    const a: MouseAction = { type: 'mouse' }
    for (const k of ['dx', 'dy', 'gotox', 'gotoy', 'click'] as const) if (h[k] != null) a[k] = h[k]
    if (h.wheelMove != null) a.wheel_move = h.wheelMove
    if (h.rightClick != null) a.right_click = h.rightClick
    if (delay != null) a.delay = String(delay)
    return a
  }
  const a: MidiAction = { type: 'midi', b0: h.b0 ?? 0, b1: h.b1 ?? 0, b2: h.b2 ?? 0 }
  if (delay != null) a.delay = delay
  return a
}

function triggerFromH(h: HTrigger, deviceType: DeviceType): Trigger {
  const kind = h.kind ?? 'tk_midi'  // omitted => 0-value tk_midi
  if (kind === 'tk_midi') {
    const t: Trigger = { type: 'midi', b0: h.b0 ?? 0, b1: h.b1 ?? 0, b2: h.b2 ?? 0 }
    // midiMode omitted on the wire => 0-value mm_normal; only set when non-normal.
    if (h.midiMode && MIDI_MODE[h.midiMode] !== 'normal') t.mode = MIDI_MODE[h.midiMode] as MidiMode
    if (h.delay != null) t.delay = h.delay
    return t
  }
  // evdev: the trigger's model type is the device kind (joystick/keyboard/mouse)
  const t: any = {
    type: deviceType as EvdevKind,
    code: h.code ?? '',
    mode: (TRIGGER_EDGE[h.edge ?? 'edge_press']) as Edge,
  }
  if (h.value != null) t.value = h.value
  if (h.erval != null) t.interval = h.erval  // wire name is "erval", see api/harpia.ts
  if (h.delay != null) t.delay = h.delay
  return t
}

function ruleFromH(h: HRule, deviceType: DeviceType): Rule {
  const r: Rule = {
    input: triggerFromH(h.input ?? {}, deviceType),
    output: (h.outputs ?? []).map(actionFromH),
  }
  if (h.changeModeEnable) r.change_mode = { enable: true, change_to: h.changeModeTo ?? 0 }
  return r
}

function modeFromH(h: HMode, deviceType: DeviceType): Mode {
  const m: Mode = {
    id: h.modeId ?? 0,
    active: !!h.active,
    actions: (h.rules ?? []).map((r) => ruleFromH(r, deviceType)),
  }
  if (h.modeHeader?.length) m.mode_header = { actions: h.modeHeader.map(actionFromH) }
  return m
}

export function boardFromH(h: HBoard): Board {
  const type = (DEVICE_TYPE[h.type ?? 'dt_midi']) as DeviceType
  return {
    DEVICE: {
      timeout: h.timeout ?? 0,
      type,
      name: h.name ?? '',
      input: h.input ?? '',
      output: h.output ?? '',
    },
    header: {
      identifier: {
        ...(h.generics ? { generics: h.generics } : {}),
        ...(h.tags ? { tags: h.tags } : {}),
        ...(h.exec ? { executable: { exec: h.exec, ...(h.port ? { port: h.port } : {}) } } : {}),
      },
      actions: (h.headerActions ?? []).map(actionFromH),
    },
    body: { modes: (h.modes ?? []).map((m) => modeFromH(m, type)) },
  }
}

// ============================ frontend -> harpia ============================
// PKs are caller-assigned and the 5 tables are shared across ALL boards, so ids
// must be unique per table. `alloc` hands out fresh ids (seed it above the current
// DB max — see client.saveBoard).

export interface Alloc {
  board(): number; mode(): number; rule(): number; trigger(): number; action(): number
}
export function counterAlloc(seed: Partial<Record<keyof Alloc, number>> = {}): Alloc {
  const n = { board: seed.board ?? 0, mode: seed.mode ?? 0, rule: seed.rule ?? 0, trigger: seed.trigger ?? 0, action: seed.action ?? 0 }
  return {
    board: () => (n.board += 1), mode: () => (n.mode += 1), rule: () => (n.rule += 1),
    trigger: () => (n.trigger += 1), action: () => (n.action += 1),
  }
}

function actionToH(a: OutputAction, id: number): HOutputAction {
  const h: HOutputAction = { [ID_KEY]: id, kind: ACTION_KIND_R[a.type] }
  if (a.type === 'midi') { h.b0 = a.b0; h.b1 = a.b1; h.b2 = a.b2; if (a.delay != null) h.delay = a.delay }
  else if (a.type === 'keyboard') {
    h.data = a.data; h.keyType = KEY_TYPE_R[a.keyType]; h.hold = HOLD_MODE_R[a.hold]
    if (a.delay != null) h.delay = a.delay
  } else {
    for (const k of ['dx', 'dy', 'gotox', 'gotoy', 'click'] as const) if (a[k] != null) h[k] = a[k]
    if (a.wheel_move != null) h.wheelMove = a.wheel_move
    if (a.right_click != null) h.rightClick = a.right_click
    if (a.delay != null) h.delay = Number(a.delay)
  }
  return h
}

function triggerToH(t: Trigger, id: number): HTrigger {
  if (t.type === 'midi') {
    const h: HTrigger = { [ID_KEY]: id, kind: 'tk_midi', b0: t.b0, b1: t.b1, b2: t.b2 }
    // Only emit midiMode when non-normal (mm_normal is the omitted 0-value).
    if (t.mode && t.mode !== 'normal') h.midiMode = MIDI_MODE_R[t.mode]
    if (t.delay != null) h.delay = t.delay
    return h
  }
  const h: HTrigger = { [ID_KEY]: id, kind: 'tk_evdev', code: t.code, edge: TRIGGER_EDGE_R[t.mode] }
  if (t.value != null) h.value = t.value
  if (t.interval != null) h.erval = t.interval  // wire name is "erval", see api/harpia.ts
  if (t.delay != null) h.delay = t.delay
  return h
}

export function boardToH(b: Board, alloc: Alloc): HBoard {
  const h: HBoard = {
    [ID_KEY]: alloc.board(),
    name: b.DEVICE.name,
    type: DEVICE_TYPE_R[b.DEVICE.type],
    timeout: b.DEVICE.timeout,
    input: b.DEVICE.input,
    output: b.DEVICE.output,
    headerActions: b.header.actions.map((a) => actionToH(a, alloc.action())),
    modes: b.body.modes.map((m) => ({
      [ID_KEY]: alloc.mode(),
      modeId: m.id,
      active: m.active ? 1 : 0,
      modeHeader: (m.mode_header?.actions ?? []).map((a) => actionToH(a, alloc.action())),
      rules: m.actions.map((r) => {
        const hr: HRule = {
          [ID_KEY]: alloc.rule(),
          input: triggerToH(r.input, alloc.trigger()),
          outputs: r.output.map((a) => actionToH(a, alloc.action())),
        }
        if (r.change_mode?.enable) { hr.changeModeEnable = 1; hr.changeModeTo = r.change_mode.change_to }
        return hr
      }),
    })),
  }
  const id = b.header.identifier
  if (id.generics) h.generics = id.generics
  if (id.tags) h.tags = id.tags
  if (id.executable?.exec) { h.exec = id.executable.exec; if (id.executable.port) h.port = id.executable.port }
  return h
}
