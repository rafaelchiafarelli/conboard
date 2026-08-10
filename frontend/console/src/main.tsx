import { StrictMode, Suspense, lazy } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App'
import ErrorBoundary from './ErrorBoundary'
import './index.css'

// Dev-only click-to-source inspector (hold Alt, click an element to copy its
// File.tsx:line). import.meta.env.DEV is a static `false` in prod, so this is
// `null` there: it stays out of the main bundle and its lazy chunk is never
// loaded. It only ever runs under `vite dev`.
const DevInspector = import.meta.env.DEV ? lazy(() => import('./DevInspector')) : null

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <ErrorBoundary>
      <App />
    </ErrorBoundary>
    {DevInspector && (
      <Suspense fallback={null}>
        <DevInspector />
      </Suspense>
    )}
  </StrictMode>,
)
