import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import { VitePWA } from 'vite-plugin-pwa';
import { version } from './package.json';

// Bump the SAMPLES_CACHE_VERSION to invalidate cached audio samples on next deploy.
// Simplest way: bump "version" in package.json.
const SAMPLES_CACHE_VERSION = version;

export default defineConfig({
  plugins: [
    react(),
    VitePWA({
      registerType: 'autoUpdate',
      // Registered manually in `main.tsx` so the desktop build (served from
      // its own local server) stays service-worker free.
      injectRegister: null,
      manifest: false, // Use existing manifest.json
      workbox: {
        cleanupOutdatedCaches: true,
        // The main bundle is well above workbox's 2 MiB default (PixiJS +
        // VexFlow + Tone.js), so allow precaching it.
        maximumFileSizeToCacheInBytes: 8 * 1024 * 1024,
        globPatterns: ['**/*.{js,css,html,ico,png,svg,woff,woff2}'],
        runtimeCaching: [
          {
            urlPattern: /\/samples\/.*\.mp3$/,
            handler: 'CacheFirst',
            options: {
              cacheName: `piano-samples-local-v${SAMPLES_CACHE_VERSION}`,
              expiration: {
                maxEntries: 500,
                maxAgeSeconds: 60 * 60 * 24 * 365, // 1 year
              },
            },
          },
          {
            urlPattern: /^https:\/\/tonejs\.github\.io\/.*\.mp3$/,
            handler: 'CacheFirst',
            options: {
              cacheName: 'piano-samples',
              expiration: {
                maxEntries: 200,
                maxAgeSeconds: 60 * 60 * 24 * 365, // 1 year
              },
            },
          },
        ],
      },
    }),
  ],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:3000',
        changeOrigin: true,
      },
    },
  },
  build: {
    outDir: '../dist/client',
    emptyOutDir: true,
    // Split the heavy renderer/audio libraries so the shell boots fast and the
    // service worker can cache each piece independently.
    chunkSizeWarningLimit: 1200,
    rollupOptions: {
      output: {
        manualChunks(id: string) {
          if (!id.includes('node_modules')) return undefined;
          if (id.includes('pixi.js') || id.includes('@pixi')) return 'pixi';
          if (id.includes('vexflow')) return 'vexflow';
          if (id.includes('tone') || id.includes('standardized-audio-context')) {
            return 'tone';
          }
          if (
            id.includes('/react/') ||
            id.includes('/react-dom/') ||
            id.includes('scheduler') ||
            id.includes('zustand')
          ) {
            return 'react';
          }
          return 'vendor';
        },
      },
    },
  },
});
