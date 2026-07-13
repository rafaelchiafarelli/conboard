// Live monitor — the dispatcher's realtime user-action stream.
//
// Frames come from the dispatcher websocket (WebSocketEventSource, /websocket ->
// dispatcher /ws) as `<uuid>,<action>` text: the sender-registration UUID plus an
// opaque device action string (INTERFACE.md O3). We show them verbatim. There is no
// simulated source — this is the real device feed only.
//
// SENDER LIST (worklist item 3): senders are shown in a STABLE, never-reordered list
// so the user can track one — each with a connection LED (lit = a frame arrived within
// CONNECTED_MS, unlit = gone quiet). A sender that goes quiet is NOT yanked out from
// under the cursor: it is pruned only later, and only when the user selects a
// different, still-connected sender (so the list never churns mid-read).
//
// KNOWN GAP: events are keyed by the dispatcher UUID; the console has no UUID -> device
// name mapping yet (the /ws payload is uuid+action). So a sender shows its UUID until
// the dispatcher adds the device name to the payload (a churn to O3).

import { useEffect, useReducer, useRef, useState } from 'react'
import { liveBus, type DeviceEvent, type LiveStatus } from './model/events'

const MAX_ROWS = 250
const CONNECTED_MS = 4000    // LED lit if a frame arrived within this window
const STALE_PRUNE_MS = 8000  // a quiet sender is eligible for pruning after this long

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

interface Sender { id: string; lastSeen: number }

export default function Monitor() {
  const [events, setEvents] = useState<DeviceEvent[]>([])
  const [paused, setPaused] = useState(false)
  const [filter, setFilter] = useState('all')  // 'all' or a sender id
  const [status, setStatus] = useState<LiveStatus>('connecting')
  const pausedRef = useRef(paused)
  pausedRef.current = paused

  // Sender registry: insertion-ordered, mutated in place so it NEVER reorders. Held in
  // a ref (identity stable across renders); a 1s tick + a bump() re-render the LEDs.
  const sendersRef = useRef<Sender[]>([])
  const [, bump] = useReducer((n: number) => n + 1, 0)

  useEffect(() => {
    // One shared socket for the whole app (liveBus); the feed is a single stream we
    // group by sender here. e.device is the devname when the dispatcher tags it (O5),
    // else the uuid.
    return liveBus.subscribe({
      onStatus: setStatus,
      onEvent: (e) => {
        if (pausedRef.current) return
        const reg = sendersRef.current
        const s = reg.find((x) => x.id === e.device)
        if (s) s.lastSeen = e.ts
        else { reg.push({ id: e.device, lastSeen: e.ts }); bump() } // new sender -> render its row
        setEvents((prev) => {
          const next = [e, ...prev]
          return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
        })
      },
    })
  }, [])

  // Re-render once a second so the connection LEDs reflect the CONNECTED_MS window.
  useEffect(() => {
    const t = setInterval(() => bump(), 1000)
    return () => clearInterval(t)
  }, [])

  const now = Date.now()
  const connected = (s: Sender) => now - s.lastSeen < CONNECTED_MS

  // Selecting a sender filters the feed. Pruning of quiet senders happens ONLY here and
  // ONLY when the newly-selected sender is itself connected (lit) — so the list stays
  // stable while you read it, and stale rows are cleaned up on your next deliberate move.
  const selectSender = (id: string) => {
    setFilter(id)
    if (id !== 'all') {
      const sel = sendersRef.current.find((s) => s.id === id)
      if (sel && connected(sel)) {
        sendersRef.current = sendersRef.current.filter(
          (s) => s.id === id || now - s.lastSeen < STALE_PRUNE_MS,
        )
      }
    }
    bump()
  }

  const senders = sendersRef.current
  const shown = filter === 'all' ? events : events.filter((e) => e.device === filter)
  const liveCount = senders.filter(connected).length

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
          <span><b>{shown.length}</b> events</span>
          <span><b className="ok">{liveCount}</b> live</span>
          <span><b className="dim">{senders.length}</b> sender{senders.length !== 1 ? 's' : ''}</span>
        </div>
        <div className="mon-controls">
          <button className="btn" onClick={() => setPaused((p) => !p)}>
            {paused ? '▶ Resume' : '⏸ Pause'}
          </button>
          <button className="btn ghost" onClick={() => { setEvents([]); sendersRef.current = []; setFilter('all'); bump() }}
                  disabled={!events.length && !senders.length}>
            Clear
          </button>
        </div>
      </div>

      {/* Stable sender list (item 3): fixed order, connection LED per sender. */}
      <div className="senders" role="tablist" aria-label="Senders">
        <button className={`sender-chip${filter === 'all' ? ' sel' : ''}`} onClick={() => selectSender('all')}>
          <span className="led all" /> all senders
        </button>
        {senders.map((s) => {
          const on = connected(s)
          return (
            <button
              key={s.id}
              className={`sender-chip${filter === s.id ? ' sel' : ''}${on ? '' : ' off'}`}
              onClick={() => selectSender(s.id)}
              title={`${s.id} — ${on ? 'connected' : 'no recent activity'}`}
            >
              <span className={`led${on ? ' on' : ''}`} />
              {short(s.id)}
            </button>
          )
        })}
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
        Real device feed from the dispatcher websocket (<b>/websocket</b> → dispatcher <b>/ws</b>). Senders are listed in
        a <b>stable order</b> with a connection <b>LED</b> (lit = active within {CONNECTED_MS / 1000}s); a quiet sender is
        pruned only after you pick another live one, so the list never reshuffles under you. Each row is a
        <b> sender UUID</b> + an opaque action string (INTERFACE.md O3) — the console has no UUID→device-name map yet, so
        the <b>fires</b> column stays blank until the dispatcher emits typed reports.
      </p>
    </div>
  )
}
