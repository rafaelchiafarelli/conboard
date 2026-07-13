// Remove-device confirmation. Delete is simple: the user already has a device
// selected, so this just confirms THAT device (name + what will be removed) — no
// re-selection. Removing deletes the profile from the library and, when it is
// deployed on the device, stops its handler + removes the on-device profile.

export default function RemoveDeviceDialog({
  deviceName,
  deviceType,
  rules,
  onCancel,
  onConfirm,
}: {
  deviceName: string
  deviceType: string
  rules: number
  onCancel: () => void
  onConfirm: () => void
}) {
  return (
    <div className="modal-scrim" onMouseDown={onCancel}>
      <div className="modal" onMouseDown={(e) => e.stopPropagation()}>
        <div className="modal-head">
          <h2>Remove device</h2>
          <button className="modal-x" onClick={onCancel} aria-label="Close">✕</button>
        </div>
        <div className="modal-body">
          <p style={{ margin: '2px 0 14px', fontSize: 14 }}>
            Remove <b>{deviceName}</b> <span className="type-badge">{deviceType}</span>?
          </p>
          <div className="ad-warn">
            Deletes its profile and {rules} rule{rules !== 1 ? 's' : ''} from the library, and stops it on the
            device (removes the on-device profile + its handler). This cannot be undone.
          </div>
        </div>
        <div className="modal-foot">
          <button className="btn ghost" onClick={onCancel}>Cancel</button>
          <button className="btn danger" onClick={onConfirm}>Remove device</button>
        </div>
      </div>
    </div>
  )
}
