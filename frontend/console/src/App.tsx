import { useEffect, useReducer, useRef, useState } from 'react'
import { BOARDS as FIXTURE_BOARDS } from './fixtures/devices'
import { BOARDS as REAL_BOARDS } from './fixtures/boards'
import { fetchBoards, saveBoard, createBoard, copyBoard, deleteBoard, ping } from './api/client'
import type { Board } from './model/rules'
import { decodeMidi } from './model/midi'
import { validateBoards, type Rule, type DeviceType } from './model/rules'
import RuleEditor from './RuleEditor'
import Monitor from './Monitor'

type View = 'rules' | 'monitor'

// Device rail grouping — one section per device kind, in a stable order.
const DEVICE_GROUPS: { type: DeviceType; label: string }[] = [
  { type: 'midi', label: 'MIDI' },
  { type: 'joystick', label: 'Joysticks' },
  { type: 'keyboard', label: 'Keyboards' },
  { type: 'mouse', label: 'Mice' },
]

/** One-line summary of a rule's trigger, for the list. */
function triggerSummary(input: Rule['input']): { badge: string; human: string; bytes: string } {
  if (input.type === 'midi') {
    const d = decodeMidi(input.b0, input.b1, input.b2)
    return { badge: d.short, human: d.human, bytes: `${input.b0} ${input.b1} ${input.b2} · ${d.detail}` }
  }
  return { badge: input.type.slice(0, 3).toUpperCase(), human: input.code, bytes: `${input.code} · ${input.mode}` }
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
  const [view, setView] = useState<View>('rules')
  const [devIdx, setDevIdx] = useState(0)
  const [modeIdx, setModeIdx] = useState(0)
  const [ruleIdx, setRuleIdx] = useState(0)
  const [dirty, setDirty] = useState(false)
  const snapshot = useRef<string | null>(null)
  const [, forceRender] = useReducer((n: number) => n + 1, 0)

  // Data source: start on the bundled fixtures, then try the backend rules-library
  // (harpia REST). Fall back to fixtures if the backend is down or empty.
  const [boards, setBoards] = useState<Board[]>(FIXTURE_BOARDS)
  const [source, setSource] = useState<'loading' | 'seeding' | 'backend' | 'fixtures'>('loading')
  // Backend id per board (parallel to `boards`); null = not yet persisted.
  const [boardIds, setBoardIds] = useState<(number | null)[]>(() => FIXTURE_BOARDS.map(() => null))
  const [saveState, setSaveState] = useState<'idle' | 'saving' | 'saved' | 'error'>('idle')

  // Index safely: backend-loaded boards may have fewer devices/modes/rules than the
  // current selection (or a board with no modes), so never assume a slot exists.
  const device = boards[devIdx] ?? boards[0]
  const modes = device?.body.modes ?? []
  const mode = modes[modeIdx] ?? modes[0]
  const rule = mode?.actions[ruleIdx] as Rule | undefined

  // Snapshot the rule as selected so Revert can restore it.
  const select = (di: number, mi: number, ri: number) => {
    setDevIdx(di)
    setModeIdx(mi)
    setRuleIdx(ri)
    const r = boards[di]?.body.modes[mi]?.actions[ri]
    snapshot.current = r ? JSON.stringify(r) : null
    setDirty(false)
  }

  // Persist the WHOLE current board to the backend. A rule cannot be saved on its own
  // (it lives inside the board aggregate), so any Save/Delete writes the whole profile;
  // saveBoard() does a delete+recreate, keeping the board's id stable. Returns silently
  // (status shown in the header) so callers can fire-and-forget.
  const persistDevice = async () => {
    setSaveState('saving')
    try {
      const id = await saveBoard(boards[devIdx], boardIds[devIdx])
      setBoardIds((ids) => { const n = ids.slice(); n[devIdx] = id; return n })
      setSaveState('saved')
    } catch (e) {
      console.error('[conboard] save failed', e)
      setSaveState('error')
    }
  }

  const onEdit = () => {
    setDirty(true)
    setSaveState('idle')
    forceRender()
  }
  const onSave = () => {
    if (rule) snapshot.current = JSON.stringify(rule)
    setDirty(false)
    void persistDevice()
  }
  const onRevert = () => {
    if (snapshot.current && mode) mode.actions[ruleIdx] = JSON.parse(snapshot.current)
    setDirty(false)
    forceRender()
  }
  const onDelete = () => {
    if (!mode) return
    mode.actions.splice(ruleIdx, 1)
    select(devIdx, modeIdx, Math.max(0, ruleIdx - 1))
    void persistDevice()   // deletion is explicit; persist the board immediately
  }
  const onAddRule = () => {
    if (!device || !mode) return
    // Trigger type always equals the device type (each board is driven by one engine).
    const input: Rule['input'] =
      device.DEVICE.type === 'midi'
        ? { type: 'midi', b0: 144, b1: 0, b2: 127 }
        : { type: device.DEVICE.type, code: device.DEVICE.type === 'keyboard' ? 'KEY_A' : 'BTN_SOUTH', mode: 'press' }
    mode.actions.push({ input, output: [] })
    select(devIdx, modeIdx, mode.actions.length - 1)
    setDirty(true)   // enable Save so the new rule can be persisted after editing
  }
  // Make a mode the device's live operation mode. Exactly one mode is active at a
  // time. This is a board edit (mode.active), so persist the whole board.
  const activateMode = (i: number) => {
    if (!device) return
    device.body.modes.forEach((m, k) => {
      m.active = k === i
    })
    forceRender()
    void persistDevice()
  }

  // ---- board-level library operations (create / copy A->B / delete) ---------
  const addBoardLocal = (b: Board, id: number | null) => {
    setBoards((bs) => [...bs, b])
    setBoardIds((ids) => [...ids, id])
    select(boards.length, 0, 0) // the appended board's index
  }
  const newBoard = async () => {
    const name = window.prompt('New device name?')?.trim()
    if (!name) return
    const b: Board = {
      DEVICE: { timeout: 0, type: 'midi', name, input: name, output: name },
      header: { identifier: {}, actions: [] },
      body: { modes: [{ id: 0, active: true, actions: [] }] },
    }
    setSaveState('saving')
    try { addBoardLocal(b, await createBoard(b)); setSaveState('saved') }
    catch (e) { console.error('[conboard] create board failed', e); setSaveState('error') }
  }
  const copyDevice = async () => {
    if (!device) return
    const name = window.prompt(`Copy "${device.DEVICE.name}" to new device name?`, `${device.DEVICE.name} copy`)?.trim()
    if (!name) return
    const clone: Board = { ...structuredClone(device), DEVICE: { ...device.DEVICE, name } }
    setSaveState('saving')
    try { addBoardLocal(clone, await copyBoard(device, { name })); setSaveState('saved') }
    catch (e) { console.error('[conboard] copy board failed', e); setSaveState('error') }
  }
  const deleteDevice = async () => {
    if (!device) return
    if (boards.length <= 1) { window.alert('Cannot delete the last device.'); return }
    if (!window.confirm(`Delete device "${device.DEVICE.name}" and all its rules?`)) return
    const id = boardIds[devIdx]
    const di = devIdx
    setSaveState('saving')
    try {
      if (id != null) await deleteBoard(id)
      setBoards((bs) => bs.filter((_, k) => k !== di))
      setBoardIds((ids) => ids.filter((_, k) => k !== di))
      select(Math.max(0, di - 1), 0, 0)
      setSaveState('saved')
    } catch (e) { console.error('[conboard] delete board failed', e); setSaveState('error') }
  }

  // Load boards from the backend rules-library once. If the backend is up but missing
  // an installed profile, SEED it from the bundled real boards using the same saveBoard
  // path the editor uses — the library starts empty (no backend seeding), so without
  // this the console would only show boards that were manually saved. Falls back to the
  // bundled fixtures when the backend is unreachable.
  useEffect(() => {
    let cancelled = false
    ;(async () => {
      try {
        if (await ping()) {
          let loaded = await fetchBoards()
          const have = new Set(loaded.map((l) => l.board.DEVICE.name))
          const missing = REAL_BOARDS.filter((b) => !have.has(b.DEVICE.name))
          if (missing.length && !cancelled) {
            setSource('seeding')
            for (const b of missing) {
              try {
                await saveBoard(b, null)
              } catch (e) {
                console.warn('[conboard] seed failed for', b.DEVICE.name, e)
              }
            }
            loaded = await fetchBoards() // re-read canonical rows + backend ids
          }
          if (!cancelled && loaded.length) {
            setBoards(loaded.map((l) => l.board))
            setBoardIds(loaded.map((l) => l.id))
            setDevIdx(0); setModeIdx(0); setRuleIdx(0)
            setSource('backend')
            return
          }
        }
      } catch (e) {
        console.warn('[conboard] backend load failed, using fixtures', e)
      }
      if (!cancelled) setSource('fixtures')
    })()
    return () => { cancelled = true }
  }, [])

  // Validate whatever is loaded: a trigger type must match its device type.
  useEffect(() => {
    const problems = validateBoards(boards)
    if (problems.length) console.warn(`[conboard] ${problems.length} invalid rule(s):\n` + problems.join('\n'))
  }, [boards])

  const liveMode = device?.body.modes.find((m) => m.active)
  const entryCount = mode?.mode_header?.actions.length ?? 0

  return (
    <div className="app">
      <header>
        <span className="mark">conboard</span>
        <span className="sub">console</span>
        <nav className="viewnav" aria-label="Views">
          <button className={view === 'rules' ? 'on' : ''} onClick={() => setView('rules')}>
            Rules
          </button>
          <button className={view === 'monitor' ? 'on' : ''} onClick={() => setView('monitor')}>
            Live monitor
          </button>
        </nav>
        <span className="stage">
          {source === 'loading' ? 'connecting…' : source === 'seeding' ? 'seeding library…' : source === 'backend' ? 'live · backend' : 'live fixtures'}
          {saveState === 'saving' && ' · saving…'}
          {saveState === 'saved' && ' · saved ✓'}
          {saveState === 'error' && ' · save failed ✕'}
        </span>
      </header>

      {view === 'monitor' && <Monitor />}
      <div className="work" hidden={view !== 'rules'}>
        <nav className="rail" aria-label="Devices">
          <span className="lbl">Devices</span>
          <div className="dev-tools">
            <button className="btn ghost" onClick={newBoard} title="Create a new device profile">＋ New</button>
            <button className="btn ghost" onClick={copyDevice} title="Copy this device's rule set to a new device (A→B)">⧉ Copy</button>
            <button className="btn danger-ghost" onClick={deleteDevice} title="Delete this device and its rules">🗑 Delete</button>
          </div>
          {DEVICE_GROUPS.map((g) => {
            const items = boards.map((b, i) => ({ b, i })).filter((x) => x.b.DEVICE.type === g.type)
            if (!items.length) return null
            return (
              <div className="rail-group" key={g.type}>
                <span className="rail-group-label">
                  {g.label} <span className="rail-group-n">{items.length}</span>
                </span>
                {items.map(({ b: d, i }) => {
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
              </div>
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
            {mode && (
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
            )}
          </div>

          <div className="list-scroll">
            <div className="list-count">
              <span>
                {(mode?.actions.length ?? 0)} rule{(mode?.actions.length ?? 0) !== 1 ? 's' : ''} · trigger → output
              </span>
            </div>
            {(mode?.actions ?? []).map((r, ri) => {
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
          {rule && mode ? (
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
                <div className="big">{mode ? `No rules in mode ${mode.id}` : 'This device has no modes'}</div>
                {mode ? 'Add a rule from the list to start mapping a trigger.' : 'Nothing to edit for this device yet.'}
              </div>
            </div>
          )}
        </main>
      </div>
    </div>
  )
}
