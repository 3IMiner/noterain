import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import App from './App';

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>,
);

// Service worker: web/PWA build only. The desktop shell ships its own files and
// serves them from a local server, so a service worker there would only add a
// stale-cache failure mode (and would re-cache the 88 MB of piano samples).
if (!window.noterainDesktop) {
  void import('virtual:pwa-register')
    .then(({ registerSW }) => {
      registerSW({ immediate: true });
    })
    .catch((error) => {
      console.warn('[App] Service worker registration failed:', error);
    });
}
