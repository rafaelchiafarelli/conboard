import { useReducer, useRef, useState } from 'react'
import { BOARDS } from './fixtures/boards'
import { decodeMidi } from './model/midi'
import type { Rule } from './model/rules'
import RuleEditor from './RuleEditor'

/** One-line summary of a rule's trigger, for the list. */
function triggerSummary(input: Rule['input']): { badge: string; human: string; bytes: string } {
  if (input.type === 'midi') {
    const d = decodeMidi(input.b0, input.b1, input.b2)
    return { badge: d.short, human: d.human, bytes: `${input.b0} ${input.b1} ${input.b2} · ${d.detail}` }
  }
  return { badge: 'EV', human: input.code, bytes: `${input.code} · ${input.edge}` }
}

/** Colored chips summarizing a rule's outputs (and its mode switch), for the list. */
function OutputSummary({ rule }: { rule: Rule }) {
  if (rule.change_mode?.enable) return <span className="chip mode">⇄ mode {rule.change_mode.change_to}</span>
  if (rule.output.length === 0) return <span className="chip none">no output</span>
  const types = [...new Set(rule.output.map((o) => o.type))]
  return (
    <>
      {types.map((t) => (
        <span key={t} className={`chip ${t === 'keyboard' ? 'kbd' : t}`}>
          {t === 'keyboard' ? 'kbd' : t}
        </span>
      ))}
      <span style={{ color: 'var(--ink-faint)' }}>{rule.output.length}×</span>
    </>
  )
}

export default function App() {
  const [devIdx, setDevIdx] = useState(0)
  const [modeIdx, setModeIdx] = useState(0)
  const [ruleIdx, setRuleIdx] = useState(0)
  const [dirty, setDirty] = useState(false)
  const snapshot = useRef<string | null>(null)
  const [, forceRender] = useReducer((n: number) => n + 1, 0)

  const device = BOARDS[devIdx]
  const mode = device.body.modes[modeIdx]
  const rule = mode.actions[ruleIdx] as Rule | undefined

  // Snapshot the rule as selected so Revert can restore it.
  const select = (di: number, mi: number, ri: number) => {
    setDevIdx(di)
    setModeIdx(mi)
    setRuleIdx(ri)
    const r = BOARDS[di].body.modes[mi].actions[ri]
    snapshot.current = r ? JSON.stringify(r) : null
    setDirty(false)
  }

  const onEdit = () => {
    setDirty(true)
    forceRender()
  }
  const onSave = () => {
    if (rule) snapshot.current = JSON.stringify(rule)
    setDirty(false)
  }
  const onRevert = () => {
    if (snapshot.current) mode.actions[ruleIdx] = JSON.parse(snapshot.current)
    setDirty(false)
    forceRender()
  }
  const onDelete = () => {
    mode.actions.splice(ruleIdx, 1)
    select(devIdx, modeIdx, Math.max(0, ruleIdx - 1))
  }
  const onAddRule = () => {
    mode.actions.push({ input: { type: 'midi', b0: 144, b1: 0, b2: 127 }, output: [] })
    select(devIdx, modeIdx, mode.actions.length - 1)
  }
  // Make a mode the device's live operation mode. Exactly one mode is active at a
  // time; on a real device this maps to a "switch mode" command sent to the backend.
  const activateMode = (i: number) => {
    device.body.modes.forEach((m, k) => {
      m.active = k === i
    })
    forceRender()
  }

  const liveMode = device.body.modes.find((m) => m.active)
  const entryCount = mode.mode_header?.actions.length ?? 0

  return (
    <div className="app">
      <header>
        <span className="mark">conboard</span>
        <span className="sub">console</span>
        <span className="stage">rule editor · live fixtures</span>
      </header>

      <div className="work">
        <nav className="rail" aria-label="Devices">
          <span className="lbl">Devices</span>
          {BOARDS.map((d, i) => {
            const live = d.body.modes.find((m) => m.active)
            return (
              <button
                key={d.DEVICE.name}
                className={`dev${i === devIdx ? ' active' : ''}`}
                onClick={() => select(i, 0, 0)}
              >
                <span className="dev-name">{d.DEVICE.name}</span>
                <span className="dev-meta">
                  <span className="type-badge">{d.DEVICE.type}</span>
                  mode {live ? live.id : '-'} live
                </span>
              </button>
            )
          })}
        </nav>

        <section className="list" aria-label="Rules">
          <div className="list-head">
            <div className="dev-title">{device.DEVICE.name}</div>
            <div className="dev-exec">
              {device.header.identifier.executable?.exec}
              {device.header.identifier.executable?.port ? ` · ${device.header.identifier.executable.port}` : ''}
            </div>
            <div className="modes">
              {device.body.modes.map((m, i) => (
                <button
                  key={m.id}
                  className={`mode-tab${i === modeIdx ? ' active' : ''}${m.active ? ' live' : ''}`}
                  onClick={() => select(devIdx, i, 0)}
                >
                  <span className="dot" />
                  mode {m.id}
                  {m.active ? ' · live' : ''}
                </button>
              ))}
            </div>
            <div className="mode-ctl">
              {mode.active ? (
                <span className="live-pill">
                  <span className="dot" />
                  mode {mode.id} is live
                </span>
              ) : (
                <>
                  <span className="live-note">Live: mode {liveMode ? liveMode.id : '—'}</span>
                  <button className="activate" onClick={() => activateMode(modeIdx)}>
                    ⏻ Activate mode {mode.id}
                  </button>
                </>
              )}
              {entryCount > 0 && (
                <span className="entry-note">
                  {entryCount} entry action{entryCount !== 1 ? 's' : ''} on enter
                </span>
              )}
            </div>
          </div>

          <div className="list-scroll">
            <div className="list-count">
              <span>
                {mode.actions.length} rule{mode.actions.length !== 1 ? 's' : ''} · trigger → output
              </span>
            </div>
            {mode.actions.map((r, ri) => {
              const t = triggerSummary(r.input)
              return (
                <button
                  key={ri}
                  className={`ritem${ri === ruleIdx ? ' sel' : ''}`}
                  onClick={() => select(devIdx, modeIdx, ri)}
                >
                  <span className="ritem-top">
                    <span className="trig-badge">{t.badge}</span>
                    <span className="ritem-human">{t.human}</span>
                  </span>
                  <span className="ritem-bytes">{t.bytes}</span>
                  <span className="ritem-out">
                    <span className="ritem-arrow">→</span> <OutputSummary rule={r} />
                  </span>
                </button>
              )
            })}
            <button className="add-rule" onClick={onAddRule}>
              ＋  Add rule
            </button>
          </div>
        </section>

        <main className="editor" aria-label="Rule editor">
          {rule ? (
            <RuleEditor
              board={device}
              modeId={mode.id}
              rule={rule}
              ruleIndex={ruleIdx}
              ruleCount={mode.actions.length}
              dirty={dirty}
              onEdit={onEdit}
              onSave={onSave}
              onRevert={onRevert}
              onDelete={onDelete}
            />
          ) : (
            <div className="empty-editor">
              <div>
                <div className="big">No rules in mode {mode.id}</div>
                Add a rule from the list to start mapping a trigger.
              </div>
            </div>
          )}
        </main>
      </div>
    </div>
  )
}
