// Per-device live-events dock (worklist item 2). Opens beside the rule list when a
// device's "live" button is toggled on in the rail. It subscribes to the shared
// liveBus (one dispatcher socket) and FILTERS the single stream to this device.
//
// Filtering needs the dispatcher to tag each frame with a device name, which it does
// via the HB roster frame (INTERFACE.md O5): once any frame resolves a devname, the
// dock shows only this device's events. Until then (legacy dispatcher) it shows all
// senders with a note — no UI change when the dispatcher starts emitting HB.

import { useEffect, useState } from 'react'
import { liveBus, type DeviceEvent, type LiveStatus } from './model/events'

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

  useEffect(() => {
    return liveBus.subscribe({
      onStatus: setStatus,
      onEvent: (e) => setEvents((prev) => {
        const next = [e, ...prev]
        return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next
      }),
    })
  }, [])

  // Filter to this device once the dispatcher tags frames with a devname (O5).
  const tagged = events.some((e) => e.devname)
  const shown = tagged ? events.filter((e) => e.devname === deviceName) : events

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
        {shown.length === 0 ? (
          <div className="feed-empty">
            {status === 'open'
              ? (tagged ? `No events from ${deviceName} yet — press a control.` : 'Waiting for actions — press a control.')
              : 'Connecting to the dispatcher…'}
          </div>
        ) : (
          shown.map((e) => (
            <div className="ld-row" key={e.id}>
              <span className="ev-time">{fmtTime(e.ts)}</span>
              <span className="ev-dev" title={e.device}>{short(e.device)}</span>
              <span className="ev-human" title={e.raw ?? ''}>{e.raw ?? e.code ?? ''}</span>
            </div>
          ))
        )}
      </div>

      <p className="livedock-note">
        {tagged
          ? <>Filtered to <b>{deviceName}</b> from the shared dispatcher feed.</>
          : <>Showing <b>all senders</b> — the dispatcher feed isn't tagged with a device name yet (INTERFACE.md O5),
             so it can't filter to {deviceName} alone. It will, with no UI change, once the dispatcher emits heartbeat frames.</>}
      </p>
    </aside>
  )
}
