import { useState } from 'react'
import { BOARDS } from './fixtures/boards'
import { decodeMidi } from './model/midi'
import type { Trigger } from './model/rules'

function triggerSummary(input: Trigger): { badge: string; human: string; bytes: string } {
  if (input.type === 'midi') {
    const d = decodeMidi(input.b0, input.b1, input.b2)
    return { badge: d.short, human: d.human, bytes: `${input.b0} ${input.b1} ${input.b2} · ${d.detail}` }
  }
  return { badge: 'EV', human: `${input.code} · ${input.edge}`, bytes: input.code }
}

export default function App() {
  const [devIdx, setDevIdx] = useState(0)
  const [modeIdx, setModeIdx] = useState(0)

  const device = BOARDS[devIdx]
  const mode = device.body.modes[modeIdx]

  return (
    <div className="app">
      <header>
        <span className="mark">conboard</span>
        <span className="sub">console</span>
        <span className="stage">step 1 · containerized skeleton · real fixtures</span>
      </header>

      <div className="work">
        <nav className="rail" aria-label="Devices">
          <h2>Devices</h2>
          {BOARDS.map((d, i) => {
            const live = d.body.modes.find((m) => m.active)
            return (
              <button
                key={d.DEVICE.name}
                className={`dev${i === devIdx ? ' active' : ''}`}
                onClick={() => {
                  setDevIdx(i)
                  setModeIdx(0)
                }}
              >
                <span className="dev-name">{d.DEVICE.name}</span>
                <span className="dev-meta">
                  <span className="type-badge">{d.DEVICE.type}</span>
                  mode {live ? live.id : '-'} live
                </span>
              </button>
            )
          })}
        </nav>

        <main className="center">
          <div className="center-head">
            <div className="title">
              {device.DEVICE.name}
              <span className="exec">{device.header.identifier.executable?.exec}</span>
            </div>
            <div className="modes">
              {device.body.modes.map((m, i) => (
                <button
                  key={m.id}
                  className={`mode-tab${i === modeIdx ? ' active' : ''}${m.active ? ' live' : ''}`}
                  onClick={() => setModeIdx(i)}
                >
                  <span className="dot" />
                  mode {m.id}
                  {m.active ? ' · active' : ''}
                </button>
              ))}
            </div>
          </div>

          <div className="rulecount">
            {mode.actions.length} rule{mode.actions.length !== 1 ? 's' : ''} · trigger → output mapping
          </div>

          <div className="rules">
            {mode.actions.map((r, ri) => {
              const t = triggerSummary(r.input)
              return (
                <div className="rule" key={ri}>
                  <span className="trigger">
                    <span className="trig-line">
                      <span className="trig-badge">{t.badge}</span>
                      <span className="trig-human">{t.human}</span>
                    </span>
                    <span className="trig-bytes">{t.bytes}</span>
                  </span>
                  <span className="arrow">→</span>
                  <span className="outs">
                    {r.change_mode ? (
                      <span className="modeswitch">⇄ switch to mode {r.change_mode.change_to}</span>
                    ) : r.output.length === 0 ? (
                      <span className="empty-out">no output</span>
                    ) : (
                      <span className="out-count">
                        {r.output.length} action{r.output.length !== 1 ? 's' : ''} ·{' '}
                        {[...new Set(r.output.map((o) => o.type))].join(' + ')}
                      </span>
                    )}
                  </span>
                </div>
              )
            })}
          </div>

          <p className="note">
            Skeleton view — proves the container boots and real board data flows through the model.
            The full trigger/output editor lands in the next steps (see the design mockup).
          </p>
        </main>
      </div>
    </div>
  )
}
