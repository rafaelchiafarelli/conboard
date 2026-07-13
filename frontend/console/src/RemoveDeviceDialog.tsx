// Remove-device flow (worklist item 4: "removal should be the same as adding").
// Mirrors AddDeviceDialog — a picker over the configured devices with a confirm —
// instead of a bare window.confirm, so add and remove feel symmetric. Removing a
// device deletes its profile from the authoring library (not the on-device realtime
// path; that changes only on the next deploy/uninstall).

import { useState } from 'react'
import type { DeviceType } from './model/rules'

export interface RemovableDevice { name: string; type: DeviceType; rules: number }

export default function RemoveDeviceDialog({
  devices,
  presetIdx,
  onCancel,
  onRemove,
}: {
  devices: RemovableDevice[]
  presetIdx: number
  onCancel: () => void
  onRemove: (idx: number) => void
}) {
  const [pick, setPick] = useState<number>(
    presetIdx >= 0 && presetIdx < devices.length ? presetIdx : -1,
  )

  return (
    <div className="modal-scrim" onMouseDown={onCancel}>
      <div className="modal add-device" onMouseDown={(e) => e.stopPropagation()}>
        <div className="modal-head">
          <h2>Remove a device</h2>
          <button className="modal-x" onClick={onCancel} aria-label="Close">✕</button>
        </div>

        <div className="modal-body">
          <div className="ad-section-label">Devices in the library</div>
          <div className="ad-list">
            {devices.length === 0 ? (
              <div className="ad-empty">No devices to remove.</div>
            ) : (
              devices.map((d, i) => (
                <button
                  key={`${d.name}-${i}`}
                  className={`ad-dev${pick === i ? ' sel' : ''}`}
                  onClick={() => setPick(i)}
                >
                  <span className="ad-dev-top">
                    <span className="ad-dev-name">{d.name}</span>
                    <span className="type-badge">{d.type}</span>
                  </span>
                  <span className="ad-dev-meta">{d.rules} rule{d.rules !== 1 ? 's' : ''}</span>
                </button>
              ))
            )}
          </div>
          {pick >= 0 && devices[pick] && (
            <div className="ad-form">
              <span className="ad-warn">
                Removing <b>{devices[pick].name}</b> deletes its profile and all its rules from the library.
                This cannot be undone.
              </span>
            </div>
          )}
        </div>

        <div className="modal-foot">
          <button className="btn ghost" onClick={onCancel}>Cancel</button>
          <button className="btn danger" disabled={pick < 0} onClick={() => pick >= 0 && onRemove(pick)}>
            Remove device
          </button>
        </div>
      </div>
    </div>
  )
}
