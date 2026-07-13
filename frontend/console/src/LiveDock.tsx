// Per-device live-events dock (worklist item 2). Opens beside the rule list when a
// device's "live" button is toggled on in the rail. Shows the dispatcher's realtime
// feed so you can watch a control fire while editing its rules.
//
// GAP (same as the full Live monitor): the /ws payload is <uuid>,<action> with no
// device name, so the feed cannot yet be filtered to THIS device — it shows every
// sender, with a note. When the dispatcher adds the device name to the payload
// (INTERFACE.md O3), this dock filters to the selected device with no UI change.

import { useEffect, useRef, useState } from 'react'
import { WebSocketEventSource, defaultWsUrl, type DeviceEvent, type LiveStatus } from './model/events'

const MAX_ROWS = 120

function fmtTime(ts: number): string {
  const d = new Date(ts)
  const p = (n: number, len = 2) => String(n).padStart(len, '0')
  return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`
}
const short = (id: string) => (id.length > 10 ? id.slice(0, 8) + '…' : id)

export default function LiveDock({ deviceName, connected, onClose }: {
  deviceName: string
  connected: boolean
  onClose: () => void
}) {
  const [events, setEvents] = useState<DeviceEvent[]>([])
  const [status, setStatus] = useState<LiveStatus>('connecting')
  const seenRef = useRef(0)

  useEffect(() => {
    const src = new WebSocketEventSource(defaultWsUrl(), setStatus)
    src.start((e) => {
      seenRef.current++
      setEvents((prev) => {
        const next = [e, ...prev]
        return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
      })
    })
    return () => src.stop()
  }, [])

  return (
    <aside className="livedock" aria-label={`Live events — ${deviceName}`}>
      <div className="livedock-head">
        <div className="livedock-title">
          <span className={`led${connected ? ' on' : ''}`} />
          <span className="livedock-name" title={deviceName}>{deviceName}</span>
          <span className={`mon-status ${status}`}><span className="pulse" />{status === 'open' ? 'live' : status}</span>
        </div>
        <button className="modal-x" onClick={onClose} aria-label="Close live view">✕</button>
      </div>

      <div className="livedock-feed">
        {events.length === 0 ? (
          <div className="feed-empty">
            {status === 'open' ? 'Waiting for actions — press a control.' : 'Connecting to the dispatcher…'}
          </div>
        ) : (
          events.map((e) => (
            <div className="ld-row" key={e.id}>
              <span className="ev-time">{fmtTime(e.ts)}</span>
              <span className="ev-dev" title={e.device}>{short(e.device)}</span>
              <span className="ev-human" title={e.raw ?? ''}>{e.raw ?? e.code ?? ''}</span>
            </div>
          ))
        )}
      </div>

      <p className="livedock-note">
        Showing <b>all senders</b> — the dispatcher feed isn't tagged with a device name yet (INTERFACE.md O3),
        so this can't filter to {deviceName} alone. It will once the dispatcher adds the name to the payload.
      </p>
    </aside>
  )
}
