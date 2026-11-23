'use strict';

/**
 * NoteRain desktop shell.
 *
 * The renderer is the existing React app (built to `dist/client`), served over
 * a loopback HTTP server so that WebMIDI, caching and normal fetch semantics
 * keep working exactly like in a browser (a `file://` page is not a secure
 * context, so WebMIDI would be unavailable there).
 */

const {
  app,
  BrowserWindow,
  Menu,
  dialog,
  ipcMain,
  session,
  shell,
} = require('electron');
const http = require('node:http');
const path = require('node:path');
const fs = require('node:fs');
const fsp = require('node:fs/promises');

const APP_ID = 'app.noterain.desktop';
const WEBSITE = 'https://noterain.app';

/** UI language of the native shell; follows the app once it reports in. */
let uiLanguage = 'en';
const isZh = () => uiLanguage === 'zh';

const CLIENT_DIR = app.isPackaged
  ? path.join(process.resourcesPath, 'client')
  : path.join(__dirname, '..', 'dist', 'client');

let mainWindow = null;
let serverPort = 0;

// ---------------------------------------------------------------------------
// Tiny static file server
// ---------------------------------------------------------------------------

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.webmanifest': 'application/manifest+json; charset=utf-8',
  '.map': 'application/json; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.webp': 'image/webp',
  '.ico': 'image/x-icon',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.ttf': 'font/ttf',
  '.otf': 'font/otf',
  '.mp3': 'audio/mpeg',
  '.ogg': 'audio/ogg',
  '.wav': 'audio/wav',
  '.mid': 'audio/midi',
  '.midi': 'audio/midi',
};

function createStaticServer(rootDir) {
  const normalizedRoot = path.normalize(rootDir);

  const server = http.createServer((req, res) => {
    void handleRequest(req, res);
  });

  async function handleRequest(req, res) {
    try {
      if (req.method !== 'GET' && req.method !== 'HEAD') {
        res.writeHead(405, { Allow: 'GET, HEAD' });
        res.end();
        return;
      }

      const url = new URL(req.url || '/', 'http://127.0.0.1');
      let pathname = decodeURIComponent(url.pathname);
      if (pathname.endsWith('/')) pathname += 'index.html';

      const filePath = path.normalize(path.join(normalizedRoot, pathname));
      if (
        filePath !== normalizedRoot &&
        !filePath.startsWith(normalizedRoot + path.sep)
      ) {
        res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('Forbidden');
        return;
      }

      let stat;
      try {
        stat = await fsp.stat(filePath);
      } catch {
        res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('Not found');
        return;
      }

      if (stat.isDirectory()) {
        res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('Not found');
        return;
      }

      const extension = path.extname(filePath).toLowerCase();
      const headers = {
        'Content-Type': MIME_TYPES[extension] || 'application/octet-stream',
        'Accept-Ranges': 'bytes',
        // Hashed assets and audio samples never change for a given build;
        // HTML stays uncached so an update is picked up on restart.
        'Cache-Control':
          pathname.startsWith('/samples/') || pathname.startsWith('/assets/')
            ? 'public, max-age=31536000, immutable'
            : 'no-cache',
      };

      const rangeHeader = req.headers.range;
      if (rangeHeader) {
        const match = /^bytes=(\d*)-(\d*)$/.exec(rangeHeader.trim());
        if (match) {
          const start = match[1] ? parseInt(match[1], 10) : 0;
          const end = match[2]
            ? Math.min(parseInt(match[2], 10), stat.size - 1)
            : stat.size - 1;
          if (start > end || start >= stat.size) {
            res.writeHead(416, {
              'Content-Range': `bytes */${stat.size}`,
            });
            res.end();
            return;
          }
          res.writeHead(206, {
            ...headers,
            'Content-Range': `bytes ${start}-${end}/${stat.size}`,
            'Content-Length': end - start + 1,
          });
          if (req.method === 'HEAD') {
            res.end();
            return;
          }
          fs.createReadStream(filePath, { start, end }).pipe(res);
          return;
        }
      }

      res.writeHead(200, { ...headers, 'Content-Length': stat.size });
      if (req.method === 'HEAD') {
        res.end();
        return;
      }
      fs.createReadStream(filePath).pipe(res);
    } catch (error) {
      console.error('[static] request failed:', error);
      if (!res.headersSent) {
        res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
      }
      res.end('Internal error');
    }
  }

  return new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => resolve(server));
  });
}

// ---------------------------------------------------------------------------
// MIDI file helpers
// ---------------------------------------------------------------------------

function isMidiPath(filePath) {
  return /\.(mid|midi)$/i.test(filePath);
}

function findMidiArg(argv) {
  for (const arg of argv) {
    if (!arg || arg.startsWith('-')) continue;
    if (!isMidiPath(arg)) continue;
    try {
      if (fs.statSync(arg).isFile()) return path.resolve(arg);
    } catch {
      // ignore stale paths
    }
  }
  return null;
}

async function readMidiFile(filePath) {
  const buffer = await fsp.readFile(filePath);
  return {
    name: path.basename(filePath),
    data: buffer.buffer.slice(
      buffer.byteOffset,
      buffer.byteOffset + buffer.byteLength,
    ),
  };
}

async function deliverMidiFile(filePath) {
  if (!mainWindow || mainWindow.isDestroyed()) return;
  try {
    const payload = await readMidiFile(filePath);
    const contents = mainWindow.webContents;
    if (contents.isLoading()) {
      contents.once('did-finish-load', () => {
        contents.send('noterain:open-midi', payload);
      });
    } else {
      contents.send('noterain:open-midi', payload);
    }
    if (mainWindow.isMinimized()) mainWindow.restore();
    mainWindow.show();
    mainWindow.focus();
  } catch (error) {
    console.error('[main] failed to open MIDI file:', filePath, error);
    dialog.showErrorBox(
      isZh() ? '无法打开文件' : 'Cannot open file',
      `${filePath}\n\n${error}`,
    );
  }
}

async function openMidiFromDialog() {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: isZh() ? '打开 MIDI 文件' : 'Open MIDI file',
    buttonLabel: isZh() ? '打开' : 'Open',
    properties: ['openFile', 'multiSelections'],
    filters: [
      { name: isZh() ? 'MIDI 文件' : 'MIDI files', extensions: ['mid', 'midi'] },
    ],
  });
  if (result.canceled) return;
  for (const filePath of result.filePaths) {
    await deliverMidiFile(filePath);
  }
}

// ---------------------------------------------------------------------------
// Window + menu
// ---------------------------------------------------------------------------

function windowIcon() {
  for (const candidate of ['icon.png', 'icon.ico']) {
    const filePath = path.join(__dirname, 'build', candidate);
    if (fs.existsSync(filePath)) return filePath;
  }
  return undefined;
}

function registerPermissionHandlers() {
  const allowed = new Set([
    'midi',
    'midiSysex',
    'media',
    'fullscreen',
    'clipboard-sanitized-write',
  ]);
  try {
    session.defaultSession.setPermissionRequestHandler(
      (_webContents, permission, callback) => {
        callback(allowed.has(permission));
      },
    );
    session.defaultSession.setPermissionCheckHandler(
      (_webContents, permission) => allowed.has(permission),
    );
  } catch (error) {
    console.warn('[main] could not register permission handlers:', error);
  }
}

function showAbout() {
  const zh = isZh();
  const detail = [
    `${zh ? '版本' : 'Version'} ${app.getVersion()}`,
    `Electron ${process.versions.electron} · Chromium ${process.versions.chrome}`,
    '',
    zh
      ? '钢琴音色：Salamander Grand Piano V3（Alexander Holm，CC BY 3.0）'
      : 'Piano samples: Salamander Grand Piano V3 (Alexander Holm, CC BY 3.0)',
    zh ? '五线谱渲染：VexFlow（MIT）' : 'Sheet music rendering: VexFlow (MIT)',
    zh ? '许可协议：GPL-3.0' : 'License: GPL-3.0',
  ].join('\n');

  void dialog.showMessageBox(mainWindow, {
    type: 'info',
    title: zh ? '关于 NoteRain' : 'About NoteRain',
    message: 'NoteRain',
    detail,
    buttons: [zh ? '确定' : 'OK'],
    noLink: true,
  });
}

function buildMenu() {
  const zh = isZh();
  const template = [
    {
      label: zh ? '文件' : 'File',
      submenu: [
        {
          label: zh ? '打开 MIDI…' : 'Open MIDI…',
          accelerator: 'CmdOrCtrl+O',
          click: () => {
            void openMidiFromDialog();
          },
        },
        { type: 'separator' },
        { role: 'quit', label: zh ? '退出' : 'Quit' },
      ],
    },
    {
      label: zh ? '编辑' : 'Edit',
      submenu: [
        { role: 'undo', label: zh ? '撤销' : 'Undo' },
        { role: 'redo', label: zh ? '重做' : 'Redo' },
        { type: 'separator' },
        { role: 'cut', label: zh ? '剪切' : 'Cut' },
        { role: 'copy', label: zh ? '复制' : 'Copy' },
        { role: 'paste', label: zh ? '粘贴' : 'Paste' },
        { role: 'selectAll', label: zh ? '全选' : 'Select all' },
      ],
    },
    {
      label: zh ? '视图' : 'View',
      submenu: [
        { role: 'reload', label: zh ? '重新加载' : 'Reload' },
        { role: 'forceReload', label: zh ? '强制重新加载' : 'Force reload' },
        {
          role: 'toggleDevTools',
          label: zh ? '开发者工具' : 'Developer tools',
        },
        { type: 'separator' },
        { role: 'resetZoom', label: zh ? '实际大小' : 'Actual size' },
        { role: 'zoomIn', label: zh ? '放大' : 'Zoom in' },
        { role: 'zoomOut', label: zh ? '缩小' : 'Zoom out' },
        { type: 'separator' },
        {
          role: 'togglefullscreen',
          label: zh ? '全屏' : 'Toggle full screen',
        },
      ],
    },
    {
      label: zh ? '帮助' : 'Help',
      submenu: [
        {
          label: zh ? '打开官网' : 'Website',
          click: () => {
            void shell.openExternal(WEBSITE);
          },
        },
        { type: 'separator' },
        {
          label: zh ? '关于 NoteRain' : 'About NoteRain',
          click: () => showAbout(),
        },
      ],
    },
  ];

  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
}

async function createWindow() {
  const server = await createStaticServer(CLIENT_DIR);
  serverPort = server.address().port;

  mainWindow = new BrowserWindow({
    width: 1360,
    height: 860,
    minWidth: 900,
    minHeight: 560,
    show: false,
    backgroundColor: '#1e2127',
    title: 'NoteRain',
    icon: windowIcon(),
    autoHideMenuBar: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      spellcheck: false,
      // Keep timers/audio at full speed even when the window is in the
      // background — playback should not stutter while reading a score.
      backgroundThrottling: false,
      additionalArguments: [`--noterain-version=${app.getVersion()}`],
    },
  });

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
  });

  mainWindow.on('closed', () => {
    mainWindow = null;
    server.close();
  });

  // External links belong in the user's browser, never inside the app shell.
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    if (/^https?:/i.test(url)) void shell.openExternal(url);
    return { action: 'deny' };
  });

  mainWindow.webContents.on('will-navigate', (event, url) => {
    if (!url.startsWith(`http://127.0.0.1:${serverPort}`)) {
      event.preventDefault();
      if (/^https?:/i.test(url)) void shell.openExternal(url);
    }
  });

  await mainWindow.loadURL(`http://127.0.0.1:${serverPort}/`);
}

// ---------------------------------------------------------------------------
// App lifecycle
// ---------------------------------------------------------------------------

const gotSingleInstanceLock = app.requestSingleInstanceLock();
if (!gotSingleInstanceLock) {
  app.quit();
} else {
  app.on('second-instance', (_event, argv) => {
    if (!mainWindow) return;
    if (mainWindow.isMinimized()) mainWindow.restore();
    mainWindow.show();
    mainWindow.focus();
    const filePath = findMidiArg(argv);
    if (filePath) void deliverMidiFile(filePath);
  });

  ipcMain.handle('noterain:pick-midi', async (event) => {
    const win = BrowserWindow.fromWebContents(event.sender) ?? mainWindow;
    const result = await dialog.showOpenDialog(win, {
      title: isZh() ? '打开 MIDI 文件' : 'Open MIDI file',
      properties: ['openFile'],
      filters: [
        {
          name: isZh() ? 'MIDI 文件' : 'MIDI files',
          extensions: ['mid', 'midi'],
        },
      ],
    });
    if (result.canceled || result.filePaths.length === 0) return null;
    return readMidiFile(result.filePaths[0]);
  });

  // The renderer owns the UI language; keep the native menu in sync with it.
  ipcMain.on('noterain:set-language', (_event, language) => {
    if (language !== 'zh' && language !== 'en') return;
    if (uiLanguage === language) return;
    uiLanguage = language;
    buildMenu();
  });

  app.whenReady().then(async () => {
    app.setAppUserModelId(APP_ID);
    uiLanguage = /^zh/i.test(app.getLocale() || '') ? 'zh' : 'en';
    registerPermissionHandlers();
    buildMenu();
    await createWindow();

    const startupFile = findMidiArg(process.argv.slice(1));
    if (startupFile) await deliverMidiFile(startupFile);

    app.on('activate', () => {
      if (BrowserWindow.getAllWindows().length === 0) {
        void createWindow();
      }
    });
  });

  app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
  });
}
