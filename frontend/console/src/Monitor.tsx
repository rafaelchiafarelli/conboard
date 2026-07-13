// Live monitor — the dispatcher's realtime user-action stream.
//
// Frames come from the dispatcher websocket (WebSocketEventSource, /websocket ->
// dispatcher /ws) as `<uuid>,<action>` text: the sender-registration UUID plus an
// opaque device action string (INTERFACE.md O3). We show them verbatim. There is no
// simulated source — this is the real device feed only.
//
// KNOWN GAP: events are keyed by the dispatcher UUID, and the console has no
// UUID -> device-name mapping yet (the dispatcher does not expose it; the ws payload
// is uuid+action). So the "sender" column shows the UUID. Mapping it to a device name
// needs a dispatcher-side addition to the /ws payload (a churn to O3).

import { useEffect, useMemo, useRef, useState } from 'react'
import {
  WebSocketEventSource,
  defaultWsUrl,
  type DeviceEvent,
  type LiveStatus,
} from './model/events'

const MAX_ROWS = 250

const STATUS_LABEL: Record<LiveStatus, string> = {
  connecting: 'connecting…',
  open: 'listening',
  closed: 'disconnected',
}

function fmtTime(ts: number): string {
  const d = new Date(ts)
  const p = (n: number, len = 2) => String(n).padStart(len, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`
}

const short = (id: string) => (id.length > 10 ? id.slice(0, 8) + '…' : id)

export default function Monitor() {
  const [events, setEvents] = useState<DeviceEvent[]>([])
  const [paused, setPaused] = useState(false)
  const [filter, setFilter] = useState('all')
  const [status, setStatus] = useState<LiveStatus>('connecting')
  const pausedRef = useRef(paused)
  pausedRef.current = paused

  useEffect(() => {
    const src = new WebSocketEventSource(defaultWsUrl(), setStatus)
    src.start((e) => {
      if (pausedRef.current) return
      setEvents((prev) => {
        const next = [e, ...prev]
        return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
      })
    })
    return () => src.stop()
  }, [])

  // Filter by the senders actually seen in the stream (uuids), not a static board list
  // — live events are keyed by uuid, so a board-name filter would never match.
  const senders = useMemo(
    () => Array.from(new Set(events.map((e) => e.device).filter(Boolean))),
    [events],
  )
  const shown = filter === 'all' ? events : events.filter((e) => e.device === filter)

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
            <b className="dim">{senders.length}</b> sender{senders.length !== 1 ? 's' : ''}
          </span>
        </div>
        <div className="mon-controls">
          <select value={filter} onChange={(e) => setFilter(e.target.value)} aria-label="Filter by sender">
            <option value="all">All senders</option>
            {senders.map((s) => (
              <option key={s} value={s}>
                {short(s)}
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
          <span>sender</span>
          <span>action</span>
          <span>uuid</span>
          <span>fires</span>
        </div>
        <div className="feed-scroll">
          {shown.length === 0 ? (
            <div className="feed-empty">
              {status === 'open'
                ? 'Connected — waiting for device actions. Press a control on the device.'
                : status === 'connecting'
                  ? 'Connecting to the dispatcher…'
                  : 'Disconnected from the dispatcher. Retrying…'}
            </div>
          ) : (
            shown.map((e) => (
              <div className="ev-row" key={e.id}>
                <span className="ev-time">{fmtTime(e.ts)}</span>
                <span className="ev-dev" title={e.device}>{short(e.device)}</span>
                <span className="ev-event">
                  <span className="trig-badge">{e.kind === 'raw' ? 'RAW' : 'EV'}</span>
                  <span className="ev-human">{e.raw ?? e.code ?? ''}</span>
                </span>
                <span className="ev-bytes" title={e.uuid ?? ''}>{short(e.uuid ?? '')}</span>
                <span className="ev-fires">
                  <span className="unmapped">—</span>
                </span>
              </div>
            ))
          )}
        </div>
      </div>

      <p className="footnote">
        Real device feed from the dispatcher websocket (<b>/websocket</b> → dispatcher <b>/ws</b>). Each row is a
        <b> sender UUID</b> + an opaque action string (INTERFACE.md O3), shown verbatim — the console has no UUID→device
        name mapping yet, and the action isn't decoded, so the <b>fires</b> column stays blank until the dispatcher
        emits typed reports. If the status reads <b>listening</b> but pressing a control adds no rows, the device
        handler isn't reporting to the dispatcher's io channel.
      </p>
    </div>
  )
}
