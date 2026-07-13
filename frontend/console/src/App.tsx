import { useEffect, useReducer, useRef, useState } from 'react'
import { BOARDS as FIXTURE_BOARDS } from './fixtures/devices'
import { BOARDS as REAL_BOARDS } from './fixtures/boards'
import { fetchBoards, saveBoard, createBoard, copyBoard, deleteBoard, deployBoard, undeployBoard, fetchDevices, ping, type AttachedDevice } from './api/client'
import type { Board } from './model/rules'
import { decodeMidi, splitStatus } from './model/midi'
import { validateBoards, type Rule, type DeviceType } from './model/rules'
import { liveBus } from './model/events'
import RuleEditor from './RuleEditor'
import Monitor from './Monitor'
import AddDeviceDialog from './AddDeviceDialog'
import RemoveDeviceDialog from './RemoveDeviceDialog'
import LiveDock from './LiveDock'

/** A board is "connected" when an attached device carries all of its match tags. */
function boardConnected(b: Board, devs: AttachedDevice[]): boolean {
  const tags = b.header.identifier.tags
  if (!tags || Object.keys(tags).length === 0) return false
  return devs.some((d) => Object.entries(tags).every(([k, v]) => d.tags[k] === v))
}

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
    const mode = input.mode && input.mode !== 'normal' ? ` · ${input.mode}` : ''
    return { badge: d.short, human: d.human, bytes: `${input.b0} ${input.b1} ${input.b2} · ${d.detail}${mode}` }
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
  const [addOpen, setAddOpen] = useState(false) // add-device dialog (item 7)
  const [removeOpen, setRemoveOpen] = useState(false) // remove-device dialog (item 4)
  const [liveDevIdx, setLiveDevIdx] = useState<number | null>(null) // per-device live dock (item 2)
  const [attached, setAttached] = useState<AttachedDevice[]>([])    // attached hw, for connection LEDs
  const [devIdx, setDevIdx] = useState(0)
  const [modeIdx, setModeIdx] = useState(0)
  const [ruleIdx, setRuleIdx] = useState(0)
  // MIDI rule-list sort + filter (a device can have hundreds of MIDI rules).
  type MidiSort = 'authored' | 'channel' | 'note' | 'velocity'
  const [midiSort, setMidiSort] = useState<MidiSort>('authored')
  const [midiFilter, setMidiFilter] = useState({ channel: '', note: '', velocity: '' })
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
  const [deployState, setDeployState] = useState<'idle' | 'deploying' | 'deployed' | 'error'>('idle')

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
    // Clear any active MIDI filter so the freshly-added rule is actually visible
    // in the list (its default trigger likely wouldn't match the current filter).
    setMidiFilter({ channel: '', note: '', velocity: '' })
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
  // Add-device flow (item 7): the dialog shows attached devices without a profile
  // and builds the Board; here we persist it and optionally deploy it right away.
  const createFromDialog = async (b: Board, deploy: boolean) => {
    setAddOpen(false)
    setSaveState('saving')
    try {
      const id = await createBoard(b)
      addBoardLocal(b, id)
      setSaveState('saved')
      if (deploy) {
        setDeployState('deploying')
        try { await deployBoard(b); setDeployState('deployed') }
        catch (e) { console.error('[conboard] deploy failed', e); setDeployState('error') }
      }
    } catch (e) { console.error('[conboard] create board failed', e); setSaveState('error') }
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
  // Axis C: push this device's saved profile to the realtime path (writes boards/*.json
  // on the device + reloads the handler). The Board model IS the boards/*.json shape.
  const deployDevice = async () => {
    if (!device) return
    setDeployState('deploying')
    try {
      const r = await deployBoard(device)
      console.info('[conboard] deployed', r)
      setDeployState('deployed')
    } catch (e) {
      console.error('[conboard] deploy failed', e)
      setDeployState('error')
    }
  }
  // Remove a device (item 4): the last device CAN be removed now (empty state is
  // handled in the render). Deletes from the backend library, then updates local
  // state and reselects a still-valid neighbour.
  const performRemove = async (idx: number) => {
    setRemoveOpen(false)
    const b = boards[idx]
    if (!b) return
    const id = boardIds[idx]
    setSaveState('saving')
    try {
      if (id != null) await deleteBoard(id)
      // Also stop it on the device (remove the realtime profile + handler). Best-effort:
      // the device may not be deployed / the backend may be off — don't block removal.
      try { await undeployBoard(b) } catch (e) { console.warn('[conboard] undeploy skipped', e) }
      const newLen = boards.length - 1
      setBoards((bs) => bs.filter((_, k) => k !== idx))
      setBoardIds((ids) => ids.filter((_, k) => k !== idx))
      // Reselect: the device that shifted into `idx`, else the last remaining one.
      const ni = newLen <= 0 ? 0 : Math.min(idx, newLen - 1)
      setDevIdx(ni); setModeIdx(0); setRuleIdx(0)
      snapshot.current = null; setDirty(false)
      // Keep the live dock pointed at the right board (indices shift on removal).
      setLiveDevIdx((li) => (li == null ? null : li === idx ? null : li > idx ? li - 1 : li))
      setSaveState('saved')
    } catch (e) {
      console.error('[conboard] delete board failed', e)
      setSaveState('error')
      window.alert(`Could not delete "${b.DEVICE.name}": ${(e as Error).message}`)
    }
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
          // Seed the library from the bundled boards ONLY when it is empty (first run).
          // Topping up "missing" boards on every load would resurrect ones the user
          // deleted — which is exactly why a deleted device used to reappear on reload.
          if (loaded.length === 0 && !cancelled) {
            setSource('seeding')
            for (const b of REAL_BOARDS) {
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

  // Poll the device inventory so each rail device can show whether its hardware is
  // attached (fallback connection signal + the add-device flow). No-op off-device
  // (the endpoint 404s -> empty list).
  useEffect(() => {
    let stop = false
    const poll = async () => {
      try { const d = await fetchDevices(); if (!stop) setAttached(d) }
      catch { if (!stop) setAttached([]) }
    }
    void poll()
    const t = setInterval(() => void poll(), 4000)
    return () => { stop = true; clearInterval(t) }
  }, [])

  // Keep the shared live socket open app-wide so the rail's per-device LEDs reflect
  // the dispatcher HEARTBEAT (O5). onTick re-renders when a heartbeat lands; the 1s
  // interval expires a LED that stopped heartbeating.
  useEffect(() => {
    const unsub = liveBus.subscribe({ onTick: () => forceRender() })
    const t = setInterval(() => forceRender(), 1000)
    return () => { unsub(); clearInterval(t) }
  }, [])

  const liveMode = device?.body.modes.find((m) => m.active)
  const entryCount = mode?.mode_header?.actions.length ?? 0

  // Build the rule-list view. Rows carry their ORIGINAL index (`ri`) so selection,
  // edit and delete always target the real slot in mode.actions regardless of how
  // the view is filtered or sorted. MIDI sort/filter only applies to MIDI devices.
  const isMidi = device?.DEVICE.type === 'midi'
  const allRules = (mode?.actions ?? []).map((r, ri) => ({ r, ri }))
  const midiFilterActive = isMidi && (midiFilter.channel !== '' || midiFilter.note !== '' || midiFilter.velocity !== '')
  let ruleRows = allRules
  if (isMidi) {
    if (midiFilterActive) {
      ruleRows = ruleRows.filter(({ r }) => {
        if (r.input.type !== 'midi') return true
        const { channel } = splitStatus(r.input.b0)
        if (midiFilter.channel !== '' && channel !== Number(midiFilter.channel)) return false
        if (midiFilter.note !== '' && r.input.b1 !== Number(midiFilter.note)) return false
        if (midiFilter.velocity !== '' && r.input.b2 !== Number(midiFilter.velocity)) return false
        return true
      })
    }
    if (midiSort !== 'authored') {
      const key = ({ r }: { r: Rule }) => {
        if (r.input.type !== 'midi') return 0
        if (midiSort === 'channel') return splitStatus(r.input.b0).channel
        if (midiSort === 'note') return r.input.b1
        return r.input.b2 // velocity
      }
      // stable sort: fall back to original index to keep equal keys authored-ordered
      ruleRows = ruleRows.slice().sort((a, b) => key(a) - key(b) || a.ri - b.ri)
    }
  }

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
            <button className="btn ghost" onClick={() => setAddOpen(true)} title="Add a device (pick an attached one or enter manually)">＋ New</button>
            <button className="btn ghost" onClick={copyDevice} title="Copy this device's rule set to a new device (A→B)">⧉ Copy</button>
            <button className="btn danger-ghost" onClick={() => setRemoveOpen(true)} title="Remove a device from the library" disabled={!boards.length}>🗑 Remove</button>
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
                  // Prefer the dispatcher heartbeat (O5); fall back to USB inventory.
                  const isConn = liveBus.isLive(d.DEVICE.name) || boardConnected(d, attached)
                  const watching = liveDevIdx === i
                  return (
                    <div className={`dev-row${i === devIdx ? ' active' : ''}`} key={d.DEVICE.name}>
                      <button className={`dev${i === devIdx ? ' active' : ''}`} onClick={() => select(i, 0, 0)}>
                        <span className="dev-name">{d.DEVICE.name}</span>
                        <span className="dev-meta">
                          <span className="type-badge">{d.DEVICE.type}</span>
                          mode {live ? live.id : '-'} live
                        </span>
                      </button>
                      {/* Per-device live button (item 2): enabled only when the hardware
                          is detected as attached; the LED lights when it's being watched. */}
                      <button
                        className={`dev-live${watching ? ' on' : ''}`}
                        disabled={!isConn}
                        title={isConn
                          ? (watching ? 'Stop watching live events' : 'Watch live events from this device')
                          : 'Device not detected as connected'}
                        onClick={() => setLiveDevIdx(watching ? null : i)}
                      >
                        <span className={`led${watching ? ' on' : ''}`} />
                        live
                      </button>
                    </div>
                  )
                })}
              </div>
            )
          })}
        </nav>

        {!device ? (
          <div className="empty-editor" style={{ flex: 1 }}>
            <div>
              <div className="big">No devices</div>
              Click <b>＋ New</b> in the rail to add one.
            </div>
          </div>
        ) : (
        <>
        <section className="list" aria-label="Rules">
          <div className="list-head">
            <div className="dev-title">{device.DEVICE.name}</div>
            <div className="dev-exec">
              {device.header.identifier.executable?.exec}
              {device.header.identifier.executable?.port ? ` · ${device.header.identifier.executable.port}` : ''}
            </div>
            <div className="deploy-ctl">
              <button className="btn primary" onClick={deployDevice} disabled={deployState === 'deploying'}
                      title="Write this profile to the device and reload its handler">
                ⇧ Deploy to device
              </button>
              <span className="deploy-status">
                {deployState === 'deploying' && 'deploying…'}
                {deployState === 'deployed' && 'deployed ✓ (handler reloaded)'}
                {deployState === 'error' && 'deploy failed ✕'}
              </span>
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
                {midiFilterActive
                  ? `${ruleRows.length} of ${allRules.length} rules`
                  : `${allRules.length} rule${allRules.length !== 1 ? 's' : ''} · trigger → output`}
              </span>
            </div>

            {isMidi && (
              // MIDI boards can carry hundreds of rules — sort/filter by the MIDI
              // fields (channel / note / velocity) to find the pertinent one.
              <div className="midi-tools">
                <div className="mt-row">
                  <label>Sort</label>
                  <select value={midiSort} onChange={(e) => setMidiSort(e.target.value as MidiSort)}>
                    <option value="authored">As authored</option>
                    <option value="channel">Channel</option>
                    <option value="note">Note (data 1)</option>
                    <option value="velocity">Velocity (data 2)</option>
                  </select>
                </div>
                <div className="mt-row filter">
                  <label>Filter</label>
                  <select value={midiFilter.channel} onChange={(e) => setMidiFilter((f) => ({ ...f, channel: e.target.value }))} title="Channel">
                    <option value="">ch: any</option>
                    {Array.from({ length: 16 }, (_, i) => (
                      <option key={i} value={i + 1}>ch {i + 1}</option>
                    ))}
                  </select>
                  <input type="number" min={0} max={127} placeholder="note" value={midiFilter.note}
                         onChange={(e) => setMidiFilter((f) => ({ ...f, note: e.target.value }))} />
                  <input type="number" min={0} max={127} placeholder="vel" value={midiFilter.velocity}
                         onChange={(e) => setMidiFilter((f) => ({ ...f, velocity: e.target.value }))} />
                  {midiFilterActive && (
                    <button className="mt-clear" onClick={() => setMidiFilter({ channel: '', note: '', velocity: '' })} title="Clear filter">✕</button>
                  )}
                </div>
              </div>
            )}

            {/* Add-rule lives at the top of the list so it stays reachable on long
                MIDI boards (hundreds of rules) without scrolling to the bottom. */}
            <button className="add-rule top" onClick={onAddRule}>
              ＋  Add rule
            </button>
            {ruleRows.map(({ r, ri }) => {
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
            {midiFilterActive && ruleRows.length === 0 && (
              <div className="list-empty">No rules match this filter.</div>
            )}
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

        {liveDevIdx != null && boards[liveDevIdx] && (
          <LiveDock
            deviceName={boards[liveDevIdx].DEVICE.name}
            connected={liveBus.isLive(boards[liveDevIdx].DEVICE.name) || boardConnected(boards[liveDevIdx], attached)}
            onClose={() => setLiveDevIdx(null)}
          />
        )}
        </>
        )}
      </div>

      {addOpen && (
        <AddDeviceDialog
          presetType={device?.DEVICE.type}
          existingNames={boards.map((b) => b.DEVICE.name)}
          onCancel={() => setAddOpen(false)}
          onCreate={createFromDialog}
        />
      )}
      {removeOpen && device && (
        <RemoveDeviceDialog
          deviceName={device.DEVICE.name}
          deviceType={device.DEVICE.type}
          rules={device.body.modes.reduce((n, m) => n + m.actions.length, 0)}
          onCancel={() => setRemoveOpen(false)}
          onConfirm={() => performRemove(devIdx)}
        />
      )}
    </div>
  )
}
