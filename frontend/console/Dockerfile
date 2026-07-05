# conboard frontend console — containerized Node toolchain.
#
# Nothing is installed on the host: node / npm / vite live only in this image, so
# the exact same dev + build environment is reproducible on any machine with
# Docker (see ../README.md). Mirrors the strategy of docker/Dockerfile (C++):
# a `scratch` artifact stage lets the static bundle be extracted for the backend
# to serve, without shipping node.
#
#   dev target   -> hot-reloading Vite dev server (used by ../docker-compose.yml)
#   build target -> `npm run build` into /app/dist
#   artifact     -> a scratch image whose filesystem IS dist/  (for `buildx --output`)
#
# NOTE: pulling node:*-alpine from Docker Hub and installing from the npm registry
# require working TLS. Behind a corporate TLS-intercepting proxy you must trust
# the proxy's root CA in the build environment (see ../README.md, "Corporate proxy").

ARG NODE_VERSION=22

# ---- dev: hot-reloading dev server -----------------------------------------
# node_modules is provided by a named volume at runtime (compose), so it never
# lands in the host tree; deps are installed on first start if the volume is empty.
FROM node:${NODE_VERSION}-alpine AS dev
WORKDIR /app
ENV NODE_ENV=development
EXPOSE 5173
CMD ["sh", "-c", "[ -d node_modules/.bin ] || npm install; exec npm run dev"]

# ---- build: reproducible production bundle ----------------------------------
FROM node:${NODE_VERSION}-alpine AS build
WORKDIR /app
# Copy manifests first so the install layer caches until dependencies change.
COPY package.json package-lock.json* ./
RUN if [ -f package-lock.json ]; then npm ci; else npm install; fi
COPY . .
RUN npm run build

# ---- artifact: static files only (scratch) ---------------------------------
# Build with: docker buildx build --target artifact -o type=local,dest=dist .
FROM scratch AS artifact
COPY --from=build /app/dist/ /
