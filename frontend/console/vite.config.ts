import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Runs inside the `console` container (see ../docker-compose.yml). The dev server
// must bind 0.0.0.0 to be reachable through the published port, and watch via
// polling because inotify events don't cross the Docker bind mount reliably.
export default defineConfig({
  plugins: [react()],
  server: {
    host: true,        // 0.0.0.0 — reachable from the host browser
    port: 5173,
    strictPort: true,
    watch: { usePolling: true },
  },
})
