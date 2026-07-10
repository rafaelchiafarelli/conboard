// Catches render-time exceptions so a crash shows a readable message instead of a
// blank (black) screen — important on the device, where there's no dev console handy.
// The message + stack are rendered on-page and logged to the console.

import { Component, type ErrorInfo, type ReactNode } from 'react'

interface State {
  error: Error | null
}

export default class ErrorBoundary extends Component<{ children: ReactNode }, State> {
  state: State = { error: null }

  static getDerivedStateFromError(error: Error): State {
    return { error }
  }

  componentDidCatch(error: Error, info: ErrorInfo) {
    console.error('[conboard] UI crashed:', error, info.componentStack)
  }

  render() {
    const { error } = this.state
    if (!error) return this.props.children
    return (
      <div className="crash">
        <div className="crash-card">
          <h1>Console stopped</h1>
          <p>The UI hit a runtime error and stopped rendering. Details below.</p>
          <pre>{error.message}{error.stack ? `\n\n${error.stack}` : ''}</pre>
          <button className="btn primary" onClick={() => location.reload()}>
            Reload
          </button>
        </div>
      </div>
    )
  }
}
