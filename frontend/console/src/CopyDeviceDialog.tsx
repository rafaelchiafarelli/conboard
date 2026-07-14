// Copy-device dialog (matches the add/remove "face"). Duplicates a device's whole
// rule set under a new name (the copy A->B library operation), replacing the old
// window.prompt.

import { useMemo, useState } from 'react'

export default function CopyDeviceDialog({
  sourceName,
  existingNames,
  onCancel,
  onCopy,
}: {
  sourceName: string
  existingNames: string[]
  onCancel: () => void
  onCopy: (newName: string) => void
}) {
  const [name, setName] = useState(`${sourceName} copy`)
  const trimmed = name.trim()
  const dup = useMemo(() => trimmed !== '' && existingNames.includes(trimmed), [trimmed, existingNames])
  const canCopy = trimmed !== '' && !dup

  return (
    <div className="modal-scrim" onMouseDown={onCancel}>
      <div className="modal" onMouseDown={(e) => e.stopPropagation()}>
        <div className="modal-head">
          <h2>Copy device</h2>
          <button className="modal-x" onClick={onCancel} aria-label="Close">✕</button>
        </div>
        <div className="modal-body">
          <p style={{ margin: '2px 0 14px', fontSize: 14 }}>
            Duplicate <b>{sourceName}</b> and all its rules under a new name.
          </p>
          <div className="field">
            <label>New device name</label>
            <input type="text" value={name} autoFocus onChange={(e) => setName(e.target.value)}
                   onKeyDown={(e) => { if (e.key === 'Enter' && canCopy) onCopy(trimmed) }} />
            {dup && <span className="ad-warn">A device with this name already exists.</span>}
          </div>
        </div>
        <div className="modal-foot">
          <button className="btn ghost" onClick={onCancel}>Cancel</button>
          <button className="btn primary" disabled={!canCopy} onClick={() => canCopy && onCopy(trimmed)}>Copy device</button>
        </div>
      </div>
    </div>
  )
}
