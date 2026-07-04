// The rule detail editor — the right pane of the master-detail console. Edits the
// selected rule's trigger, its ordered output actions, and its optional mode switch.
// It mutates the rule object in place and calls onEdit() so the parent can flag the
// rule dirty and re-render; persistence to the backend is wired later (INTERFACE.md).

import type {
  Board,
  Rule,
  MidiTrigger,
  EvdevTrigger,
  KeyboardAction,
  MidiAction,
  MouseAction,
  KeyType,
  HoldMode,
  OutputAction,
} from './model/rules'
import { decodeMidi, prettyKey, MESSAGE_NAMES, isCC, splitStatus, makeStatus } from './model/midi'

const KEY_TYPES: KeyType[] = ['text', 'oneKey', 'hotKey']
const HOLD_MODES: HoldMode[] = ['not_hold', 'hold', 'hold_once']
const EDGES: EvdevTrigger['edge'][] = ['press', 'release', 'hold', 'hold_once', 'higher', 'lower', 'spot']

export default function RuleEditor({
  board,
  modeId,
  rule,
  ruleIndex,
  ruleCount,
  dirty,
  onEdit,
  onSave,
  onRevert,
  onDelete,
}: {
  board: Board
  modeId: number
  rule: Rule
  ruleIndex: number
  ruleCount: number
  dirty: boolean
  onEdit: () => void
  onSave: () => void
  onRevert: () => void
  onDelete: () => void
}) {
  return (
    <>
      <div className="ed-head">
        <div>
          <h1>
            Rule {ruleIndex + 1} <span style={{ color: 'var(--ink-faint)', fontWeight: 400 }}>of {ruleCount}</span>
          </h1>
          <div className="ed-sub">
            {board.DEVICE.name} · mode {modeId}
          </div>
        </div>
        <div className="ed-actions">
          <button className="btn danger-ghost" onClick={onDelete}>
            Delete
          </button>
          <button className="btn ghost" onClick={onRevert} disabled={!dirty}>
            Revert
          </button>
          <button className="btn primary" onClick={onSave} disabled={!dirty}>
            Save rule
          </button>
        </div>
      </div>

      <TriggerSection rule={rule} onEdit={onEdit} />
      <OutputsSection rule={rule} onEdit={onEdit} />
      <ModeSwitchSection board={board} rule={rule} onEdit={onEdit} />

      <p className="footnote">
        Editing the live configuration in memory — changes mutate the fixture model only; persistence wires to the
        backend later via the <b>INTERFACE.md</b> contract. <b>Save</b>/<b>Revert</b> enable once a field changes.
        <br />
        Model: <b>console/src/model/rules.ts</b> · trigger decode: <b>console/src/model/midi.ts</b>
      </p>
    </>
  )
}

/* ---------------- trigger ---------------- */

function TriggerSection({ rule, onEdit }: { rule: Rule; onEdit: () => void }) {
  const input = rule.input
  const isMidi = input.type === 'midi'

  const setType = (type: 'midi' | 'evdev') => {
    if (type === input.type) return
    rule.input =
      type === 'midi'
        ? ({ type: 'midi', b0: 144, b1: 0, b2: 127 } as MidiTrigger)
        : ({ type: 'evdev', code: 'BTN_A', edge: 'press' } as EvdevTrigger)
    onEdit()
  }

  return (
    <div className="section trigger">
      <div className="sec-head">
        <span className="accent-bar" />
        <span className="lbl">Trigger — rule input</span>
        <span className="seg trig">
          <button className={isMidi ? 'on' : ''} onClick={() => setType('midi')}>
            MIDI
          </button>
          <button className={!isMidi ? 'on' : ''} onClick={() => setType('evdev')}>
            evdev
          </button>
        </span>
      </div>
      <div className="sec-body">
        {input.type === 'midi' ? (
          <MidiTriggerFields input={input} onEdit={onEdit} />
        ) : (
          <EvdevTriggerFields input={input} onEdit={onEdit} />
        )}
      </div>
    </div>
  )
}

function MidiTriggerFields({ input, onEdit }: { input: MidiTrigger; onEdit: () => void }) {
  const { channel } = splitStatus(input.b0)
  const cc = isCC(input.b0)
  const d = decodeMidi(input.b0, input.b1, input.b2)

  const patch = (p: Partial<MidiTrigger>) => {
    Object.assign(input, p)
    onEdit()
  }
  const clamp = (v: string) => Math.max(0, Math.min(127, Number(v) || 0))

  return (
    <>
      <div className="fields">
        <div className="field">
          <label>Message</label>
          <select
            value={input.b0 & 0xf0}
            onChange={(e) => patch({ b0: makeStatus(Number(e.target.value), channel) })}
          >
            {Object.entries(MESSAGE_NAMES).map(([hex, name]) => (
              <option key={hex} value={Number(hex)}>
                {name}
              </option>
            ))}
          </select>
        </div>
        <div className="field">
          <label>Channel</label>
          <select value={channel} onChange={(e) => patch({ b0: makeStatus(input.b0 & 0xf0, Number(e.target.value)) })}>
            {Array.from({ length: 16 }, (_, i) => (
              <option key={i} value={i + 1}>
                ch {i + 1}
              </option>
            ))}
          </select>
        </div>
        <div className="field">
          <label>{cc ? 'CC number' : 'Note (data 1)'}</label>
          <input type="number" min={0} max={127} value={input.b1} onChange={(e) => patch({ b1: clamp(e.target.value) })} />
        </div>
        <div className="field">
          <label>{cc ? 'Value' : 'Velocity (data 2)'}</label>
          <input type="number" min={0} max={127} value={input.b2} onChange={(e) => patch({ b2: clamp(e.target.value) })} />
        </div>
      </div>
      <div className="bytechip">
        raw bytes <span className="raw">{input.b0} {input.b1} {input.b2}</span> ·{' '}
        <b>
          {d.human} · {d.detail}
        </b>
      </div>
    </>
  )
}

function EvdevTriggerFields({ input, onEdit }: { input: EvdevTrigger; onEdit: () => void }) {
  const patch = (p: Partial<EvdevTrigger>) => {
    Object.assign(input, p)
    onEdit()
  }
  return (
    <div className="fields">
      <div className="field">
        <label>Event code</label>
        <input type="text" value={input.code} placeholder="BTN_A / KEY_ENTER / ABS_X" onChange={(e) => patch({ code: e.target.value })} />
        <span className="hint">evdev symbolic code</span>
      </div>
      <div className="field">
        <label>Edge</label>
        <select value={input.edge} onChange={(e) => patch({ edge: e.target.value as EvdevTrigger['edge'] })}>
          {EDGES.map((edge) => (
            <option key={edge}>{edge}</option>
          ))}
        </select>
      </div>
    </div>
  )
}

/* ---------------- outputs ---------------- */

function OutputsSection({ rule, onEdit }: { rule: Rule; onEdit: () => void }) {
  const add = (type: OutputAction['type']) => {
    if (type === 'keyboard') rule.output.push({ type: 'keyboard', keyType: 'text', data: '', hold: 'not_hold' })
    else if (type === 'mouse') rule.output.push({ type: 'mouse' })
    else rule.output.push({ type: 'midi', b0: 144, b1: 0, b2: 127 })
    onEdit()
  }
  const remove = (i: number) => {
    rule.output.splice(i, 1)
    onEdit()
  }
  const move = (i: number, dir: -1 | 1) => {
    const j = i + dir
    ;[rule.output[i], rule.output[j]] = [rule.output[j], rule.output[i]]
    onEdit()
  }

  const modeOnly = rule.change_mode?.enable && rule.output.length === 0

  return (
    <div className="section outputs">
      <div className="sec-head">
        <span className="accent-bar" />
        <span className="lbl">Output actions — fired in order</span>
      </div>
      <div className="sec-body">
        {rule.output.length === 0 ? (
          <div className="empty-out">
            No output actions. {modeOnly ? 'This rule only switches mode.' : 'Add one below, or set a mode switch.'}
          </div>
        ) : (
          rule.output.map((out, i) => (
            <OutputRow
              key={i}
              out={out}
              index={i}
              total={rule.output.length}
              onEdit={onEdit}
              onRemove={() => remove(i)}
              onMove={(dir) => move(i, dir)}
            />
          ))
        )}
        <div className="add-out">
          <button className="kbd" onClick={() => add('keyboard')}>
            <span className="d" />＋ keyboard
          </button>
          <button className="mouse" onClick={() => add('mouse')}>
            <span className="d" />＋ mouse
          </button>
          <button className="midi" onClick={() => add('midi')}>
            <span className="d" />＋ midi
          </button>
        </div>
      </div>
    </div>
  )
}

function OutputRow({
  out,
  index,
  total,
  onEdit,
  onRemove,
  onMove,
}: {
  out: OutputAction
  index: number
  total: number
  onEdit: () => void
  onRemove: () => void
  onMove: (dir: -1 | 1) => void
}) {
  const badge = out.type === 'keyboard' ? { cls: 'kbd', label: 'KBD' } : out.type === 'mouse' ? { cls: 'mouse', label: 'MOUSE' } : { cls: 'midi', label: 'MIDI' }

  return (
    <div className="out-row">
      <div className="grip">
        <button onClick={() => onMove(-1)} disabled={index === 0} title="Move up" aria-label="Move up">
          ▲
        </button>
        <span className="idx">{index + 1}</span>
        <button onClick={() => onMove(1)} disabled={index === total - 1} title="Move down" aria-label="Move down">
          ▼
        </button>
      </div>
      <div className="out-type">
        <span className={`chip ${badge.cls}`}>{badge.label}</span>
      </div>
      <div className="out-fields">
        {out.type === 'keyboard' && <KeyboardFields out={out} onEdit={onEdit} />}
        {out.type === 'midi' && <MidiOutputFields out={out} onEdit={onEdit} />}
        {out.type === 'mouse' && <MouseFields out={out} onEdit={onEdit} />}
      </div>
      <button className="btn danger-ghost rm" onClick={onRemove} title="Remove output" aria-label="Remove output">
        ✕
      </button>
    </div>
  )
}

function KeyboardFields({ out, onEdit }: { out: KeyboardAction; onEdit: () => void }) {
  const patch = (p: Partial<KeyboardAction>) => {
    Object.assign(out, p)
    onEdit()
  }
  return (
    <>
      <div className="field">
        <label>Key type</label>
        <select value={out.keyType} onChange={(e) => patch({ keyType: e.target.value as KeyType })}>
          {KEY_TYPES.map((k) => (
            <option key={k}>{k}</option>
          ))}
        </select>
      </div>
      <div className="field" style={{ gridColumn: 'span 2' }}>
        <label>{out.keyType === 'text' ? 'Text to type' : 'Key token(s)'}</label>
        <input type="text" value={out.data} onChange={(e) => patch({ data: e.target.value })} />
        {out.keyType !== 'text' && out.data ? <span className="hint">{prettyKey(out.data)}</span> : null}
      </div>
      <div className="field">
        <label>Hold</label>
        <select value={out.hold} onChange={(e) => patch({ hold: e.target.value as HoldMode })}>
          {HOLD_MODES.map((h) => (
            <option key={h}>{h}</option>
          ))}
        </select>
      </div>
    </>
  )
}

function MidiOutputFields({ out, onEdit }: { out: MidiAction; onEdit: () => void }) {
  const d = decodeMidi(out.b0, out.b1, out.b2)
  const patch = (p: Partial<MidiAction>) => {
    Object.assign(out, p)
    onEdit()
  }
  const num = (v: string, max: number) => Math.max(0, Math.min(max, Number(v) || 0))
  return (
    <>
      <div className="field">
        <label>b0 status</label>
        <input type="number" min={0} max={255} value={out.b0} onChange={(e) => patch({ b0: num(e.target.value, 255) })} />
      </div>
      <div className="field">
        <label>b1 data</label>
        <input type="number" min={0} max={127} value={out.b1} onChange={(e) => patch({ b1: num(e.target.value, 127) })} />
      </div>
      <div className="field">
        <label>b2 data</label>
        <input type="number" min={0} max={127} value={out.b2} onChange={(e) => patch({ b2: num(e.target.value, 127) })} />
      </div>
      <div className="field">
        <label>Decoded</label>
        <span className="hint" style={{ paddingTop: 9 }}>
          {d.human} · {d.detail}
        </span>
      </div>
    </>
  )
}

function MouseFields({ out, onEdit }: { out: MouseAction; onEdit: () => void }) {
  const patch = (p: Partial<MouseAction>) => {
    Object.assign(out, p)
    onEdit()
  }
  return (
    <>
      <div className="field">
        <label>dx</label>
        <input type="text" value={out.dx ?? ''} onChange={(e) => patch({ dx: e.target.value })} />
      </div>
      <div className="field">
        <label>dy</label>
        <input type="text" value={out.dy ?? ''} onChange={(e) => patch({ dy: e.target.value })} />
      </div>
      <div className="field">
        <label>wheel</label>
        <input type="text" value={out.wheel_move ?? ''} onChange={(e) => patch({ wheel_move: e.target.value })} />
      </div>
      <div className="checkline">
        <label>
          <input
            type="checkbox"
            checked={out.click === 'true'}
            onChange={(e) => patch({ click: e.target.checked ? 'true' : undefined })}
          />{' '}
          left click
        </label>
        <label>
          <input
            type="checkbox"
            checked={out.right_click === 'true'}
            onChange={(e) => patch({ right_click: e.target.checked ? 'true' : undefined })}
          />{' '}
          right click
        </label>
      </div>
    </>
  )
}

/* ---------------- mode switch ---------------- */

function ModeSwitchSection({ board, rule, onEdit }: { board: Board; rule: Rule; onEdit: () => void }) {
  const on = !!rule.change_mode?.enable
  const target = on ? rule.change_mode!.change_to : board.body.modes[0]?.id ?? 0

  const toggle = (checked: boolean) => {
    rule.change_mode = checked ? { enable: true, change_to: target } : undefined
    onEdit()
  }
  const setTarget = (id: number) => {
    if (rule.change_mode) rule.change_mode.change_to = id
    onEdit()
  }

  return (
    <div className={`section modeswitch${on ? '' : ' off'}`}>
      <div className="sec-head">
        <span className="accent-bar" />
        <span className="lbl">Mode switch</span>
      </div>
      <div className="sec-body">
        <div className="toggle-row">
          <label className="toggle">
            <input type="checkbox" checked={on} onChange={(e) => toggle(e.target.checked)} />
            <span className="track" />
          </label>
          <span className="desc">
            On trigger, <b>switch the device to another mode</b>. Output actions above still fire first.
          </span>
        </div>
        <div className="field target">
          <label>Change to</label>
          <select value={target} onChange={(e) => setTarget(Number(e.target.value))} disabled={!on}>
            {board.body.modes.map((m) => (
              <option key={m.id} value={m.id}>
                mode {m.id}
                {m.active ? ' · live' : ''}
              </option>
            ))}
          </select>
        </div>
      </div>
    </div>
  )
}
