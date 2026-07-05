// Live monitor — a feed of incoming device events, each tagged with the rule it
// would fire in the device's live mode. Fed by SimulatedEventSource today; the same
// view works unchanged once a real websocket source lands (see model/events.ts).

import { useEffect, useMemo, useRef, useState } from 'react'
import { BOARDS } from './fixtures/devices'
import { decodeMidi } from './model/midi'
import { SimulatedEventSource, matchEvent, type DeviceEvent } from './model/events'
import type { Rule } from './model/rules'

const MAX_ROWS = 250

const byName = (name: string) => BOARDS.find((b) => b.DEVICE.name === name)

function fmtTime(ts: number): string {
  const d = new Date(ts)
  const p = (n: number, len = 2) => String(n).padStart(len, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`
}

function MatchCell({ rule }: { rule: Rule | undefined }) {
  if (!rule) return <span className="unmapped">unmapped</span>
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
      <span className="match-n">{rule.output.length}×</span>
    </>
  )
}

export default function Monitor() {
  const [events, setEvents] = useState<DeviceEvent[]>([])
  const [paused, setPaused] = useState(false)
  const [filter, setFilter] = useState('all')
  const pausedRef = useRef(paused)
  pausedRef.current = paused

  useEffect(() => {
    const src = new SimulatedEventSource(BOARDS)
    src.start((e) => {
      if (pausedRef.current) return
      setEvents((prev) => {
        const next = [e, ...prev]
        return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
      })
    })
    return () => src.stop()
  }, [])

  const shown = filter === 'all' ? events : events.filter((e) => e.device === filter)
  const matched = useMemo(() => {
    let n = 0
    for (const e of shown) {
      const b = byName(e.device)
      if (b && matchEvent(b, e)) n++
    }
    return n
  }, [shown])

  return (
    <div className="monitor">
      <div className="mon-bar">
        <div className="mon-title">
          Live monitor
          <span className={`mon-status${paused ? ' paused' : ''}`}>
            <span className="pulse" />
            {paused ? 'paused' : 'listening'}
          </span>
        </div>
        <div className="mon-stats">
          <span>
            <b>{shown.length}</b> events
          </span>
          <span>
            <b className="ok">{matched}</b> matched
          </span>
          <span>
            <b className="dim">{shown.length - matched}</b> unmapped
          </span>
        </div>
        <div className="mon-controls">
          <select value={filter} onChange={(e) => setFilter(e.target.value)} aria-label="Filter by device">
            <option value="all">All devices</option>
            {BOARDS.map((b) => (
              <option key={b.DEVICE.name} value={b.DEVICE.name}>
                {b.DEVICE.name}
              </option>
            ))}
          </select>
          <button className="btn" onClick={() => setPaused((p) => !p)}>
            {paused ? '▶ Resume' : '⏸ Pause'}
          </button>
          <button className="btn ghost" onClick={() => setEvents([])} disabled={!events.length}>
            Clear
          </button>
        </div>
      </div>

      <div className="feed">
        <div className="feed-head">
          <span>time</span>
          <span>device</span>
          <span>event</span>
          <span>bytes</span>
          <span>fires</span>
        </div>
        <div className="feed-scroll">
          {shown.length === 0 ? (
            <div className="feed-empty">
              {paused ? 'Paused — resume to see incoming events.' : 'Listening… waiting for device events.'}
            </div>
          ) : (
            shown.map((e) => {
              const board = byName(e.device)
              const rule = board ? matchEvent(board, e) : undefined
              const d = e.kind === 'midi' ? decodeMidi(e.b0!, e.b1!, e.b2!) : null
              return (
                <div className={`ev-row${rule ? ' hit' : ''}`} key={e.id}>
                  <span className="ev-time">{fmtTime(e.ts)}</span>
                  <span className="ev-dev">{e.device}</span>
                  <span className="ev-event">
                    <span className="trig-badge">{d ? d.short : 'EV'}</span>
                    <span className="ev-human">{d ? `${d.human} · ${d.detail}` : `${e.code} · ${e.edge}`}</span>
                  </span>
                  <span className="ev-bytes">{e.kind === 'midi' ? `${e.b0} ${e.b1} ${e.b2}` : e.code}</span>
                  <span className="ev-fires">
                    <span className="ev-arrow">→</span> <MatchCell rule={rule} />
                  </span>
                </div>
              )
            })
          )}
        </div>
      </div>

      <p className="footnote">
        Simulated event stream — mostly replays real mapped triggers from each device's live mode, with some unmapped
        noise. The <b>fires</b> column shows what the live mode would run. Swaps to the dispatcher websocket later via
        <b> model/events.ts</b> (INTERFACE.md).
      </p>
    </div>
  )
}
