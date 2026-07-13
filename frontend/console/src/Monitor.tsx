// Full live monitor (all devices) — the single "on top" view, toggled from the header
// (item 6). Reads the shared liveBus (one dispatcher socket, INTERFACE.md).
//
// SENDER LIST (items 3/4): senders are shown in a STABLE, never-reordered list. A
// device shows up as soon as it is CONNECTED (a heartbeat/roster frame arrived, O5) —
// not only once it sends an event — and is labelled by its device NAME + type when the
// dispatcher tags it, falling back to the raw UUID otherwise. The connection LED is lit
// while the device is live; a quiet sender is pruned only later and only when the user
// selects a different, still-live one, so the list never churns mid-read.

import { useEffect, useReducer, useRef, useState } from 'react'
import { liveBus, type DeviceEvent, type LiveStatus } from './model/events'
import type { Board } from './model/rules'

const MAX_ROWS = 250
const CONNECTED_MS = 4000
const STALE_PRUNE_MS = 8000

const STATUS_LABEL: Record<LiveStatus, string> = {
  connecting: 'connecting…', open: 'listening', closed: 'disconnected',
}

function fmtTime(ts: number): string {
  const d = new Date(ts)
  const p = (n: number, len = 2) => String(n).padStart(len, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`
}
const short = (id: string) => (id.length > 10 ? id.slice(0, 8) + '…' : id)

interface Sender { id: string; lastSeen: number }

export default function Monitor({ boards, onClose }: { boards: Board[]; onClose: () => void }) {
  const [events, setEvents] = useState<DeviceEvent[]>([])
  const [paused, setPaused] = useState(false)
  const [filter, setFilter] = useState('all')
  const [status, setStatus] = useState<LiveStatus>('connecting')
  const pausedRef = useRef(paused)
  pausedRef.current = paused

  const sendersRef = useRef<Sender[]>([])
  const [, bump] = useReducer((n: number) => n + 1, 0)

  useEffect(() => {
    return liveBus.subscribe({
      onStatus: setStatus,
      // A connected device (heartbeat) appears even before it sends an event.
      onTick: () => {
        const reg = sendersRef.current
        let added = false
        liveBus.liveness.forEach((ts, devname) => {
          if (!reg.find((s) => s.id === devname)) { reg.push({ id: devname, lastSeen: ts }); added = true }
        })
        if (added) bump()
      },
      onEvent: (e) => {
        if (pausedRef.current) return
        const reg = sendersRef.current
        const s = reg.find((x) => x.id === e.device)
        if (s) s.lastSeen = e.ts
        else { reg.push({ id: e.device, lastSeen: e.ts }); bump() }
        setEvents((prev) => {
          const next = [e, ...prev]
          return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
        })
      },
    })
  }, [])

  useEffect(() => {
    const t = setInterval(() => bump(), 1000)
    return () => clearInterval(t)
  }, [])

  const now = Date.now()
  // Connected = a heartbeat within the window (O5), or recent stream activity (fallback).
  const connected = (s: Sender) => liveBus.isLive(s.id, CONNECTED_MS) || now - s.lastSeen < CONNECTED_MS
  // A sender id is a devname (when tagged) — resolve its type from the configured boards.
  const typeOf = (id: string) => boards.find((b) => b.DEVICE.name === id)?.DEVICE.type

  const selectSender = (id: string) => {
    setFilter(id)
    if (id !== 'all') {
      const sel = sendersRef.current.find((s) => s.id === id)
      if (sel && connected(sel)) {
        sendersRef.current = sendersRef.current.filter((s) => s.id === id || (!connected(s) ? now - s.lastSeen < STALE_PRUNE_MS : true))
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
          <span className={`mon-status ${status}`}><span className="pulse" />{STATUS_LABEL[status]}</span>
        </div>
        <div className="mon-stats">
          <span><b>{shown.length}</b> events</span>
          <span><b className="ok">{liveCount}</b> live</span>
          <span><b className="dim">{senders.length}</b> sender{senders.length !== 1 ? 's' : ''}</span>
        </div>
        <div className="mon-controls">
          <button className="btn" onClick={() => setPaused((p) => !p)}>{paused ? '▶ Resume' : '⏸ Pause'}</button>
          <button className="btn ghost" onClick={() => { setEvents([]); sendersRef.current = []; setFilter('all'); bump() }}
                  disabled={!events.length && !senders.length}>Clear</button>
          <button className="btn ghost" onClick={onClose} title="Close the live monitor">✕ Close</button>
        </div>
      </div>

      {/* Stable sender list (items 3/4): device name + type + connection LED, larger. */}
      <div className="senders" role="tablist" aria-label="Senders">
        <button className={`sender-chip${filter === 'all' ? ' sel' : ''}`} onClick={() => selectSender('all')}>
          <span className="led all" /> all senders
        </button>
        {senders.map((s) => {
          const on = connected(s)
          const type = typeOf(s.id)
          return (
            <button
              key={s.id}
              className={`sender-chip big${filter === s.id ? ' sel' : ''}${on ? '' : ' off'}`}
              onClick={() => selectSender(s.id)}
              title={`${s.id} — ${on ? 'connected' : 'no recent activity'}`}
            >
              <span className={`led${on ? ' on' : ''}`} />
              <span className="sender-name">{short(s.id)}</span>
              {type && <span className="type-badge">{type}</span>}
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
                : status === 'connecting' ? 'Connecting to the dispatcher…' : 'Disconnected from the dispatcher. Retrying…'}
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
                <span className="ev-fires"><span className="unmapped">—</span></span>
              </div>
            ))
          )}
        </div>
      </div>

      <p className="footnote">
        Real device feed from the dispatcher websocket (<b>/websocket</b> → dispatcher <b>/ws</b>). A device appears here
        as soon as it's <b>connected</b> (heartbeat, INTERFACE.md O5) and is shown by <b>name + type</b> when the
        dispatcher tags it, else by its raw UUID. Until the dispatcher emits heartbeat frames, senders appear on their
        first event and the <b>fires</b> column stays blank.
      </p>
    </div>
  )
}
