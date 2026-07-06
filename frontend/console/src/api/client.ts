// Thin REST client for the conboard backend (harpia-generated CRUD). Attaches the
// per-entity credential headers and maps records to/from the frontend model.
//
// Base URL: VITE_CONBOARD_API (e.g. "http://board.local:8080/api/v1"), else "/api/v1"
// (served same-origin behind nginx). The X-Pswd is the compile-time domain hash, not a
// secret; conboard's real auth (the power-password design) layers in front separately.
import type { Board } from '../model/rules'
import { HASH, ID_KEY, type Entity, type HBoard } from './harpia'
import { boardFromH, boardToH, counterAlloc, type Alloc } from './map'

const BASE: string =
  (import.meta as any).env?.VITE_CONBOARD_API?.replace(/\/$/, '') ?? '/api/v1'

function headers(entity: Entity, body: boolean): Record<string, string> {
  const h: Record<string, string> = { 'X-User': entity, 'X-Pswd': HASH }
  if (body) h['Content-Type'] = 'application/json'
  return h
}

async function req(entity: Entity, method: string, path: string, body?: unknown): Promise<Response> {
  const res = await fetch(`${BASE}/${entity}${path}`, {
    method,
    headers: headers(entity, body !== undefined),
    body: body !== undefined ? JSON.stringify(body) : undefined,
  })
  if (!res.ok) throw new Error(`${method} ${entity}${path} -> ${res.status}`)
  return res
}

async function listRaw(entity: Entity): Promise<any[]> {
  const res = await req(entity, 'GET', '')
  const txt = await res.text()
  return txt ? JSON.parse(txt) : []
}

/** Health check — resolves true if the backend answers. */
export async function ping(): Promise<boolean> {
  try {
    const base = BASE.replace(/\/api\/v1$/, '')
    const res = await fetch(`${base}/healthz`)
    return res.ok
  } catch { return false }
}

/** Load every board as the nested frontend model (GET /board returns the full aggregate). */
export async function listBoards(): Promise<Board[]> {
  return (await listRaw('board')).map((h) => boardFromH(h as HBoard))
}

/** A board plus its backend primary key, so the UI can target it for update/delete. */
export interface LoadedBoard { board: Board; id: number }
export async function fetchBoards(): Promise<LoadedBoard[]> {
  return (await listRaw('board')).map((h) => ({
    board: boardFromH(h as HBoard),
    id: Number((h as any)[ID_KEY] ?? 0),
  }))
}

/** Delete a board aggregate by id (204). */
export async function deleteBoard(id: number): Promise<void> {
  await req('board', 'DELETE', `/${id}`)
}

/** Highest existing id per table, so a save can allocate fresh, globally-unique PKs. */
async function maxIds(): Promise<Partial<Record<keyof Alloc, number>>> {
  const max = (rows: any[]) => rows.reduce((m, r) => Math.max(m, Number(r?.[ID_KEY] ?? 0)), 0)
  const [board, mode, rule, trigger, action] = await Promise.all([
    listRaw('board'), listRaw('mode'), listRaw('rule'), listRaw('trigger'), listRaw('output_action'),
  ])
  return { board: max(board), mode: max(mode), rule: max(rule), trigger: max(trigger), action: max(action) }
}

/**
 * Persist a whole board aggregate.
 *
 * The generated update() only UPDATEs existing child rows (it will not INSERT rules/
 * modes/actions the edit added), so structural edits can't go through PUT. Instead we
 * DELETE the old aggregate and CREATE a fresh one, pinning the board's own id (when it
 * already exists) so its identity is stable, and allocating fresh, globally-unique PKs
 * for every child (above the current per-table maxima). Orphaned child rows from the
 * old version are left behind (harmless: reads follow the board's link tables), a minor
 * storage leak acceptable for an authoring library.
 *
 * @param existingId the board's current backend id, or null to create a new one.
 * @returns the board's id after the save.
 */
export async function saveBoard(board: Board, existingId: number | null = null): Promise<number> {
  const seed = await maxIds()
  if (existingId != null) {
    try { await deleteBoard(existingId) } catch { /* may not exist yet; ignore */ }
  }
  const h = boardToH(board, counterAlloc(seed))
  if (existingId != null) h[ID_KEY] = existingId  // keep the board's identity stable
  await req('board', 'POST', '', h)
  return h[ID_KEY] as number
}

/** Create a new board (fresh id). */
export function createBoard(board: Board): Promise<number> {
  return saveBoard(board, null)
}

/** Copy a board's rule set under a new identity (the "copy A->B" library operation). */
export function copyBoard(source: Board, overrides: Partial<Board['DEVICE']> = {}): Promise<number> {
  const clone: Board = { ...source, DEVICE: { ...source.DEVICE, ...overrides } }
  return createBoard(clone)
}
