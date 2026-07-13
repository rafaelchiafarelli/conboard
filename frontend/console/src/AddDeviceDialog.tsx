// Add-device flow (worklist item 7). Instead of blindly prompting for a name/type,
// we show the devices actually attached that have NO profile yet ("undesignated"),
// plus a "not on the list" manual fallback. Picking a device pre-fills its name and
// detected type; the user confirms/edits, and OK builds a Board profile whose
// header.identifier.tags come from the device identity (so the launcher will match
// it) and whose exec matches the type. The parent persists + optionally deploys it.

import { useEffect, useMemo, useState } from 'react'
import { fetchDevices, type AttachedDevice } from './api/client'
import type { Board, DeviceType } from './model/rules'

const TYPES: DeviceType[] = ['midi', 'joystick', 'keyboard', 'mouse']
const EXEC: Record<DeviceType, string> = {
  midi: '/conboard/LowLevel/MIDI/build/conMIDI',
  joystick: '/conboard/LowLevel/Joystick/build/conJoyS',
  keyboard: '/conboard/LowLevel/KeyBoard/build/conKeyB',
  mouse: '/conboard/LowLevel/Mouse/build/conMouse',
}

// Dev-only stand-ins so the flow is exercisable in `npm run dev` with no backend.
const MOCK: AttachedDevice[] = [
  { name: 'Xbox 360 Controller', type: 'joystick', vid: '045e', pid: '028e', serial: '', devpath: '/devices/…/3-1.2',
    designated: false, profile: '', tags: { ID_MODEL: 'Xbox_360_Controller', ID_VENDOR_ID: '045e', ID_MODEL_ID: '028e', ID_BUS: 'usb' } },
  { name: 'nanoKONTROL2', type: 'midi', vid: '0944', pid: '0117', serial: '', devpath: '/devices/…/3-1.3',
    designated: false, profile: '', tags: { ID_MODEL: 'nanoKONTROL2', ID_VENDOR_ID: '0944', ID_MODEL_ID: '0117', ID_BUS: 'usb' } },
]

function buildBoard(name: string, type: DeviceType, tags: Record<string, string>): Board {
  const hasTags = Object.keys(tags).length > 0
  return {
    DEVICE: { timeout: 0, type, name, input: name, output: name },
    header: {
      identifier: {
        ...(hasTags ? { tags } : {}),
        executable: { exec: EXEC[type] },
      },
      actions: [],
    },
    body: { modes: [{ id: 0, active: true, actions: [] }] },
  }
}

export default function AddDeviceDialog({
  presetType,
  existingNames,
  onCancel,
  onCreate,
}: {
  presetType?: DeviceType
  existingNames: string[]
  onCancel: () => void
  onCreate: (board: Board, deploy: boolean) => void
}) {
  const [devices, setDevices] = useState<AttachedDevice[] | null>(null)
  const [source, setSource] = useState<'loading' | 'live' | 'mock' | 'offline'>('loading')
  // selection: an attached device, or 'manual', or null (nothing picked yet)
  const [pick, setPick] = useState<AttachedDevice | 'manual' | null>(null)
  const [name, setName] = useState('')
  const [type, setType] = useState<DeviceType | ''>(presetType ?? '')

  useEffect(() => {
    let cancelled = false
    ;(async () => {
      try {
        const all = await fetchDevices()
        if (cancelled) return
        setDevices(all.filter((d) => !d.designated))
        setSource('live')
      } catch {
        if (cancelled) return
        // Dev server: show mocks so the flow can be exercised; on a real board a
        // failed fetch just means "no inventory" -> manual entry only.
        if ((import.meta as { env?: { DEV?: boolean } }).env?.DEV) { setDevices(MOCK); setSource('mock') }
        else { setDevices([]); setSource('offline') }
      }
    })()
    return () => { cancelled = true }
  }, [])

  const choose = (d: AttachedDevice | 'manual') => {
    setPick(d)
    if (d === 'manual') { setName(''); setType(presetType ?? '') }
    else { setName(d.name); setType(d.type || presetType || '') }
  }

  const dupName = useMemo(
    () => name.trim() !== '' && existingNames.includes(name.trim()),
    [name, existingNames],
  )
  const canCreate = name.trim() !== '' && type !== '' && !dupName

  const submit = () => {
    if (!canCreate) return // canCreate implies type !== '' -> type narrows to DeviceType
    const isReal = pick !== null && pick !== 'manual'
    // Auto-deploy only when the picked device is actually attached; a manual entry is
    // just a library template with no hardware to deploy to.
    onCreate(buildBoard(name.trim(), type, isReal ? pick.tags : {}), isReal)
  }

  return (
    <div className="modal-scrim" onMouseDown={onCancel}>
      <div className="modal add-device" onMouseDown={(e) => e.stopPropagation()}>
        <div className="modal-head">
          <h2>Add a device</h2>
          <button className="modal-x" onClick={onCancel} aria-label="Close">✕</button>
        </div>

        <div className="modal-body">
          <div className="ad-section-label">
            Attached devices without a profile
            <span className={`ad-src ${source}`}>
              {source === 'loading' ? 'scanning…' : source === 'live' ? 'live' : source === 'mock' ? 'demo data' : 'backend offline'}
            </span>
          </div>

          <div className="ad-list">
            {devices === null ? (
              <div className="ad-empty">Scanning attached devices…</div>
            ) : devices.length === 0 ? (
              <div className="ad-empty">
                {source === 'offline'
                  ? 'No device inventory (backend unreachable). Use “Not on the list”.'
                  : 'No undesignated devices attached. Use “Not on the list” to add one manually.'}
              </div>
            ) : (
              devices.map((d, i) => (
                <button
                  key={`${d.devpath}-${i}`}
                  className={`ad-dev${pick !== 'manual' && pick === d ? ' sel' : ''}`}
                  onClick={() => choose(d)}
                >
                  <span className="ad-dev-top">
                    <span className="ad-dev-name">{d.name}</span>
                    {d.type ? <span className="type-badge">{d.type}</span> : <span className="ad-dev-unknown">unclassified</span>}
                  </span>
                  <span className="ad-dev-meta">
                    {d.vid && d.pid ? `${d.vid}:${d.pid}` : 'no VID/PID'}{d.serial ? ` · ${d.serial}` : ''}
                  </span>
                </button>
              ))
            )}
            <button
              className={`ad-dev manual${pick === 'manual' ? ' sel' : ''}`}
              onClick={() => choose('manual')}
            >
              <span className="ad-dev-top"><span className="ad-dev-name">Not on the list</span></span>
              <span className="ad-dev-meta">enter a device manually</span>
            </button>
          </div>

          {pick !== null && (
            <div className="ad-form">
              <div className="field">
                <label>Device name</label>
                <input type="text" value={name} autoFocus onChange={(e) => setName(e.target.value)}
                       placeholder="e.g. DJ-Tech 4-Mix" />
                {dupName && <span className="ad-warn">A device with this name already exists.</span>}
              </div>
              <div className="field">
                <label>Type {pick !== 'manual' && pick.type ? '(detected)' : ''}</label>
                <select value={type} onChange={(e) => setType(e.target.value as DeviceType)}>
                  <option value="" disabled>choose a type…</option>
                  {TYPES.map((t) => <option key={t} value={t}>{t}</option>)}
                </select>
              </div>
              {pick !== null && pick !== 'manual' && (
                <span className="hint">This device is attached — it'll be deployed to the board automatically.</span>
              )}
            </div>
          )}
        </div>

        <div className="modal-foot">
          <button className="btn ghost" onClick={onCancel}>Cancel</button>
          <button className="btn primary" disabled={!canCreate} onClick={submit}>Add device</button>
        </div>
      </div>
    </div>
  )
}
