# ---- deps ----------------------------------------------------------------
FROM node:22-bookworm-slim AS deps
WORKDIR /app

# Backend and client are installed separately (no npm workspaces, so the image
# builds on any filesystem).
COPY package.json ./
COPY client/package.json ./client/
RUN npm install --no-audit --no-fund \
    && npm --prefix client install --no-audit --no-fund

# ---- build ---------------------------------------------------------------
FROM node:22-bookworm-slim AS builder
WORKDIR /app

COPY --from=deps /app/node_modules ./node_modules
COPY --from=deps /app/client/node_modules ./client/node_modules
COPY . .

# Build backend (NestJS) and client (Vite)
RUN npm run build

# ---- runtime -------------------------------------------------------------
FROM node:22-bookworm-slim AS runner
WORKDIR /app

ENV NODE_ENV=production

RUN groupadd --system --gid 1001 nodejs \
    && useradd --system --uid 1001 --gid nodejs nestjs

# Copy built backend, then install only production dependencies
COPY --from=builder --chown=nestjs:nodejs /app/dist ./dist
COPY --chown=nestjs:nodejs package.json ./
RUN npm install --omit=dev --no-audit --no-fund

USER nestjs

EXPOSE 3000

ENV PORT=3000
ENV FORCE_HTTP=true

CMD ["node", "dist/main.js"]
