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
import { decodeMidi, MESSAGE_NAMES, isCC, splitStatus, makeStatus } from './model/midi'
import {
  KEY_TOKEN_GROUPS,
  codeGroupsFor,
  MODIFIER_TOKENS,
  keyLabel,
  codeLabel,
  comboLabel,
  splitCombo,
  joinCombo,
  edgesFor,
  isAxis,
  type Edge,
} from './model/hid'

const KEY_TYPES: KeyType[] = ['text', 'oneKey', 'hotKey']
const HOLD_MODES: HoldMode[] = ['not_hold', 'hold', 'hold_once']

const CUSTOM = '__custom__'

/** A grouped <select> over a token/code catalog, with a "— custom —" escape to raw text. */
function GroupedSelect({
  value,
  groups,
  labelFn,
  onPick,
  placeholder,
}: {
  value: string
  groups: { label: string; items: string[] }[]
  labelFn: (v: string) => string
  onPick: (v: string) => void
  placeholder?: string
}) {
  const known = groups.some((g) => g.items.includes(value))
  return (
    <>
      <select value={known ? value : CUSTOM} onChange={(e) => onPick(e.target.value === CUSTOM ? '' : e.target.value)}>
        {groups.map((g) => (
          <optgroup key={g.label} label={g.label}>
            {g.items.map((it) => (
              <option key={it} value={it}>
                {labelFn(it)}
              </option>
            ))}
          </optgroup>
        ))}
        <option value={CUSTOM}>— custom —</option>
      </select>
      {!known && (
        <input type="text" value={value} placeholder={placeholder ?? 'raw token'} onChange={(e) => onPick(e.target.value)} />
      )}
    </>
  )
}

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
  // The trigger kind is fixed by the device's engine (MIDI vs evdev), not editable
  // per rule — a MIDI device can only ever fire MIDI triggers, and vice versa.
  return (
    <div className="section trigger">
      <div className="sec-head">
        <span className="accent-bar" />
        <span className="lbl">Trigger — rule input</span>
        <span className="trig-kind" title="Determined by the device's input type">
          {input.type === 'midi' ? 'MIDI' : 'evdev'}
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
  const codeGroups = codeGroupsFor(input.type).map((g) => ({ label: g.label, items: g.codes }))
  const edges = edgesFor(input.code)
  return (
    <div className="fields">
      <div className="field">
        <label>Event code</label>
        <GroupedSelect
          value={input.code}
          groups={codeGroups}
          labelFn={codeLabel}
          placeholder="BTN_A / KEY_ENTER / ABS_X"
          onPick={(code) => {
            const next: Partial<EvdevTrigger> = { code }
            // Keep the edge (mode) valid for the new code's category (buttons vs axes).
            if (code && !edgesFor(code).includes(input.mode as Edge)) next.mode = edgesFor(code)[0]
            patch(next)
          }}
        />
        <span className="hint">evdev symbolic code</span>
      </div>
      <div className="field">
        <label>Edge</label>
        <select value={input.mode} onChange={(e) => patch({ mode: e.target.value as Edge })}>
          {edges.map((edge) => (
            <option key={edge}>{edge}</option>
          ))}
        </select>
        <span className="hint">{isAxis(input.code) ? 'axis magnitude' : 'button edge'}</span>
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
  const tokenGroups = KEY_TOKEN_GROUPS.map((g) => ({ label: g.label, items: g.tokens }))
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
      {out.keyType === 'text' && (
        <div className="field" style={{ gridColumn: 'span 2' }}>
          <label>Text to type</label>
          <input type="text" value={out.data} onChange={(e) => patch({ data: e.target.value })} />
        </div>
      )}
      {out.keyType === 'oneKey' && (
        <div className="field" style={{ gridColumn: 'span 2' }}>
          <label>Key</label>
          <GroupedSelect value={out.data} groups={tokenGroups} labelFn={keyLabel} onPick={(t) => patch({ data: t })} />
        </div>
      )}
      {out.keyType === 'hotKey' && (
        <div className="field" style={{ gridColumn: '1 / -1' }}>
          <label>Hotkey combo</label>
          <HotKeyBuilder data={out.data} onChange={(d) => patch({ data: d })} />
        </div>
      )}
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

/** Modifier toggles + a base-key picker that compose a hotKey token string. */
function HotKeyBuilder({ data, onChange }: { data: string; onChange: (d: string) => void }) {
  const { mods, base } = splitCombo(data)
  const baseGroups = KEY_TOKEN_GROUPS.filter((g) => g.label !== 'Modifiers').map((g) => ({ label: g.label, items: g.tokens }))
  const toggle = (m: string) => {
    const next = mods.includes(m) ? mods.filter((x) => x !== m) : [...mods, m]
    onChange(joinCombo(next, base))
  }
  return (
    <div className="hotkey">
      <div className="mod-toggles">
        {MODIFIER_TOKENS.map((m) => (
          <button key={m} type="button" className={`modtog${mods.includes(m) ? ' on' : ''}`} onClick={() => toggle(m)}>
            {keyLabel(m)}
          </button>
        ))}
      </div>
      <div className="hotkey-key">
        <span className="plus">+</span>
        <GroupedSelect value={base} groups={baseGroups} labelFn={keyLabel} onPick={(t) => onChange(joinCombo(mods, t))} />
      </div>
      {data.trim() && (
        <span className="hint">
          {comboLabel(data)} · <code>{data}</code>
        </span>
      )}
    </div>
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
