// Live monitor — a feed of incoming device events. Two sources, same UI:
//  • Live: the dispatcher's websocket user-action stream (WebSocketEventSource). Frames
//    are opaque `<uuid>,<action>` strings (INTERFACE.md O3), shown verbatim.
//  • Simulated: SimulatedEventSource, for demoing without a device. These are structured
//    and get matched against the live mode's rules (the "fires" column).

import { useEffect, useMemo, useRef, useState } from 'react'
import { BOARDS } from './fixtures/devices'
import { decodeMidi } from './model/midi'
import {
  SimulatedEventSource,
  WebSocketEventSource,
  matchEvent,
  defaultWsUrl,
  type DeviceEvent,
  type LiveStatus,
} from './model/events'
import type { Rule } from './model/rules'

const MAX_ROWS = 250
type Source = 'live' | 'sim'
type Status = LiveStatus | 'sim'

const byName = (name: string) => BOARDS.find((b) => b.DEVICE.name === name)

function fmtTime(ts: number): string {
  const d = new Date(ts)
  const p = (n: number, len = 2) => String(n).padStart(len, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`
}

const STATUS_LABEL: Record<Status, string> = {
  connecting: 'connecting…',
  open: 'listening',
  closed: 'disconnected',
  sim: 'simulated',
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
  const [source, setSource] = useState<Source>('live')
  const [status, setStatus] = useState<Status>('connecting')
  const pausedRef = useRef(paused)
  pausedRef.current = paused

  // (Re)subscribe whenever the source changes. Switching clears the feed.
  useEffect(() => {
    setEvents([])
    const src =
      source === 'live'
        ? new WebSocketEventSource(defaultWsUrl(), setStatus)
        : new SimulatedEventSource(BOARDS)
    if (source === 'sim') setStatus('sim')
    src.start((e) => {
      if (pausedRef.current) return
      setEvents((prev) => {
        const next = [e, ...prev]
        return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
      })
    })
    return () => src.stop()
  }, [source])

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
          <span className={`mon-status ${status}`}>
            <span className="pulse" />
            {STATUS_LABEL[status]}
          </span>
        </div>
        <div className="mon-stats">
          <span>
            <b>{shown.length}</b> events
          </span>
          <span>
            <b className="ok">{matched}</b> matched
          </span>
        </div>
        <div className="mon-controls">
          <span className="seg">
            <button className={source === 'live' ? 'on' : ''} onClick={() => setSource('live')}>
              Live
            </button>
            <button className={source === 'sim' ? 'on' : ''} onClick={() => setSource('sim')}>
              Simulated
            </button>
          </span>
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
          <span>{source === 'live' ? 'sender' : 'device'}</span>
          <span>event</span>
          <span>{source === 'live' ? 'uuid' : 'bytes'}</span>
          <span>fires</span>
        </div>
        <div className="feed-scroll">
          {shown.length === 0 ? (
            <div className="feed-empty">
              {source === 'live'
                ? status === 'open'
                  ? 'Connected — waiting for device actions.'
                  : status === 'connecting'
                    ? 'Connecting to the dispatcher…'
                    : 'Disconnected from the dispatcher. Retrying… (or switch to Simulated).'
                : paused
                  ? 'Paused — resume to see incoming events.'
                  : 'Listening… waiting for device events.'}
            </div>
          ) : (
            shown.map((e) => {
              const board = byName(e.device)
              const rule = board ? matchEvent(board, e) : undefined
              const d = e.kind === 'midi' ? decodeMidi(e.b0!, e.b1!, e.b2!) : null
              const badge = e.kind === 'raw' ? 'RAW' : d ? d.short : 'EV'
              const human = e.kind === 'raw' ? e.raw : d ? `${d.human} · ${d.detail}` : `${e.code} · ${e.edge}`
              const aux = e.kind === 'raw' ? e.uuid ?? '' : e.kind === 'midi' ? `${e.b0} ${e.b1} ${e.b2}` : e.code
              return (
                <div className={`ev-row${rule ? ' hit' : ''}`} key={e.id}>
                  <span className="ev-time">{fmtTime(e.ts)}</span>
                  <span className="ev-dev">{e.device}</span>
                  <span className="ev-event">
                    <span className="trig-badge">{badge}</span>
                    <span className="ev-human">{human}</span>
                  </span>
                  <span className="ev-bytes">{aux}</span>
                  <span className="ev-fires">
                    {e.kind === 'raw' ? (
                      <span className="unmapped">—</span>
                    ) : (
                      <>
                        <span className="ev-arrow">→</span> <MatchCell rule={rule} />
                      </>
                    )}
                  </span>
                </div>
              )
            })
          )}
        </div>
      </div>

      <p className="footnote">
        {source === 'live' ? (
          <>
            Live stream from the dispatcher websocket (<b>/websocket</b> → dispatcher <b>/ws</b>). Actions are opaque
            device strings (INTERFACE.md O3), shown verbatim — not matched to rules. Switch to <b>Simulated</b> to demo
            rule matching without a device.
          </>
        ) : (
          <>
            Simulated stream — mostly replays real mapped triggers from each device's live mode, with some unmapped
            noise. The <b>fires</b> column shows what the live mode would run. Switch to <b>Live</b> for the real
            dispatcher feed.
          </>
        )}
      </p>
    </div>
  )
}
