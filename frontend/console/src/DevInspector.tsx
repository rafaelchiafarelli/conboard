import { useEffect, useRef, useState } from 'react'

/**
 * Dev-only "click-to-source" inspector.
 *
 * Hold Alt (Option) and move the mouse: the element under the cursor is
 * highlighted and a label shows its source location (File.tsx:line). Alt-click
 * copies that reference to the clipboard and pins it in the badge, so it can be
 * pasted verbatim to describe exactly which element to change.
 *
 * How it works: in dev, @vitejs/plugin-react annotates every JSX element with a
 * `__source` ({ fileName, lineNumber }) which React 18 keeps on the fiber as
 * `_debugSource`. We read it straight off the DOM node's fiber — no extra deps.
 *
 * This whole module is mounted only under `import.meta.env.DEV` (see main.tsx),
 * so it is tree-shaken out of production builds.
 */

type Source = { fileName: string; lineNumber: number; columnNumber?: number }

function getFiber(node: Node): any | null {
  const key = Object.keys(node).find(
    (k) => k.startsWith('__reactFiber$') || k.startsWith('__reactInternalInstance$'),
  )
  return key ? (node as any)[key] : null
}

/** Walk up the fiber tree from a DOM node to the nearest JSX source location. */
function getSource(node: Node): Source | null {
  let fiber = getFiber(node)
  while (fiber) {
    if (fiber._debugSource) return fiber._debugSource as Source
    fiber = fiber.return
  }
  return null
}

/** Absolute path -> short repo-relative-ish label, e.g. "src/RuleEditor.tsx". */
function shortPath(fileName: string): string {
  const i = fileName.lastIndexOf('/src/')
  if (i !== -1) return fileName.slice(i + 1) // "src/RuleEditor.tsx"
  const slash = fileName.lastIndexOf('/')
  return slash !== -1 ? fileName.slice(slash + 1) : fileName
}

function ref(src: Source): string {
  return `${shortPath(src.fileName)}:${src.lineNumber}`
}

export default function DevInspector() {
  const [active, setActive] = useState(false)
  const [hover, setHover] = useState<{ rect: DOMRect; label: string } | null>(null)
  const [pinned, setPinned] = useState<string | null>(null)
  const [copied, setCopied] = useState(false)
  const lastEl = useRef<Element | null>(null)

  // Alt toggles "pick" mode on/off (keydown starts it, keyup ends it).
  useEffect(() => {
    const down = (e: KeyboardEvent) => {
      if (e.key === 'Alt') setActive(true)
    }
    const up = (e: KeyboardEvent) => {
      if (e.key === 'Alt') {
        setActive(false)
        setHover(null)
      }
    }
    // If focus leaves the window while Alt is held, don't get stuck active.
    const blur = () => {
      setActive(false)
      setHover(null)
    }
    window.addEventListener('keydown', down)
    window.addEventListener('keyup', up)
    window.addEventListener('blur', blur)
    return () => {
      window.removeEventListener('keydown', down)
      window.removeEventListener('keyup', up)
      window.removeEventListener('blur', blur)
    }
  }, [])

  useEffect(() => {
    if (!active) return

    const move = (e: MouseEvent) => {
      const el = document.elementFromPoint(e.clientX, e.clientY)
      if (!el || el === lastEl.current) return
      // Ignore our own overlay chrome.
      if ((el as HTMLElement).dataset?.devInspector !== undefined) return
      lastEl.current = el
      const src = getSource(el)
      if (!src) {
        setHover(null)
        return
      }
      setHover({ rect: el.getBoundingClientRect(), label: ref(src) })
    }

    const click = (e: MouseEvent) => {
      const el = document.elementFromPoint(e.clientX, e.clientY)
      if (!el) return
      const src = getSource(el)
      if (!src) return
      e.preventDefault()
      e.stopPropagation()
      const r = ref(src)
      setPinned(r)
      // Clipboard may be blocked on non-secure origins (http over LAN IP); the
      // pinned badge is always selectable as a fallback.
      navigator.clipboard?.writeText(r).then(
        () => {
          setCopied(true)
          setTimeout(() => setCopied(false), 1200)
        },
        () => setCopied(false),
      )
      // eslint-disable-next-line no-console
      console.log('[inspector]', r)
    }

    window.addEventListener('mousemove', move, true)
    window.addEventListener('click', click, true)
    return () => {
      window.removeEventListener('mousemove', move, true)
      window.removeEventListener('click', click, true)
    }
  }, [active])

  const box = hover?.rect
  return (
    <div data-dev-inspector="" style={{ position: 'fixed', inset: 0, zIndex: 2147483647, pointerEvents: 'none' }}>
      {active && box && (
        <>
          <div
            style={{
              position: 'fixed',
              left: box.left,
              top: box.top,
              width: box.width,
              height: box.height,
              border: '1px solid #4f9dff',
              background: 'rgba(79,157,255,0.15)',
              boxSizing: 'border-box',
              pointerEvents: 'none',
            }}
          />
          <div
            style={{
              position: 'fixed',
              left: box.left,
              top: box.top > 22 ? box.top - 22 : box.bottom + 4,
              font: '12px/18px ui-monospace, SFMono-Regular, Menlo, monospace',
              background: '#4f9dff',
              color: '#fff',
              padding: '1px 6px',
              borderRadius: 3,
              whiteSpace: 'nowrap',
              pointerEvents: 'none',
            }}
          >
            {hover.label}
          </div>
        </>
      )}

      {/* Persistent badge: shows how to use it, and the last pinned reference. */}
      <div
        data-dev-inspector=""
        style={{
          position: 'fixed',
          bottom: 8,
          right: 8,
          font: '11px/16px ui-monospace, SFMono-Regular, Menlo, monospace',
          background: active ? '#4f9dff' : 'rgba(20,22,28,0.85)',
          color: '#fff',
          padding: '4px 8px',
          borderRadius: 5,
          pointerEvents: 'auto',
          userSelect: 'text',
          boxShadow: '0 1px 4px rgba(0,0,0,0.4)',
          maxWidth: 320,
        }}
      >
        <div style={{ opacity: 0.75 }}>
          {active ? 'inspect: move mouse, click to copy' : 'hold Alt to inspect elements'}
        </div>
        {pinned && (
          <div style={{ marginTop: 2, fontWeight: 600 }}>
            {pinned} {copied ? '✓ copied' : ''}
          </div>
        )}
      </div>
    </div>
  )
}
