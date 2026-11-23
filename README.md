<p align="center">
  <img src="assets/logo_title.png" alt="NoteRain" width="400" />
</p>

<p align="center">
  <strong><a href="https://noterain.app">noterain.app</a></strong>
</p>

A modern web-based MIDI player and piano learning tool with falling notes visualization, sheet music rendering, and MIDI keyboard support.

Runs as a web app **and** as a standalone Windows desktop application (`.exe`), with a full **简体中文 / English** interface.

## 衍生版本说明 / Derivative notice

本仓库是基于 **NoteRain**（[noterain.app](https://noterain.app)，GPL-3.0）上游源码修改的衍生版本，修改时间 2026-11。
主要改动：

- 完整简体中文界面（`client/src/i18n/`，默认跟随系统语言）
- 新增 **C++ 原生桌面版**（[`noterain-native/`](noterain-native/)，Direct2D + WASAPI + winmm，无第三方依赖）
- 新增 Electron 打包与独立 exe（[`desktop/`](desktop/)、[`release/`](release/)）
- 小键盘音域窗口：25/32/37/49 键控制器可用（窗口跟随、键位对齐、音域外自动伴奏）
- 构建脚本从 bun 改为 npm，前后端分开安装；Dockerfile 同步重写
- 练习模式改进：等待按键的分数统计、错音反馈、循环后自动提速

原项目版权归原作者所有；本衍生版同样以 **GPL-3.0** 发布，见 [LICENSE](LICENSE)。

## Screenshots

![Falling Notes](images/fallingnotes.png)

![Sheet Music](images/notesheet.png)

## Features

### Playback Controls
- **Play/Pause/Stop** - Full transport controls for MIDI playback
- **Speed Control** - Adjust playback speed from 10% to 200%
- **Seek** - Click on the progress bar or scroll through sheet music to navigate
- **Measure Looping** - Set start and end measures to practice specific sections
- **Wait Mode** - Pauses playback until you play the correct notes on your MIDI keyboard

### Visualization

#### Falling Notes
- Animated falling notes display (Synthesia-style)
- Configurable lookahead time
- Note name labels with accidentals
- Glow effect on active notes
- Grid overlay aligned to piano keys

#### Sheet Music
- Real-time staff notation rendering with VexFlow
- Automatic clef selection (treble/bass) per track
- Key signature detection and proper accidental handling
- Time signature support with correct beam grouping
- Multi-track grand staff display
- Auto-scroll during playback
- Scroll-to-seek when paused

#### Piano Keyboard
- Full 88-key piano display
- Interactive keys (click/touch to play)
- Visual feedback for playback and live input
- Note name labels
- Fit to song option (shows only the note range used in the song)

### Track Management
- **Enable/Disable Tracks** - Toggle tracks for playback
- **Render Only** - Display a track visually without playing audio (useful for learning)
- **Audio Toggle** - Mute/unmute individual tracks
- **Color Coding** - Each track has a distinct color

### Note Coloring
- **By Track** - Notes colored by their track
- **By Pitch** - Rainbow spectrum based on note pitch

### MIDI Input
- WebMIDI support for hardware keyboards
- Automatic device detection
- Works with wait mode for practice
- Live note visualization on keyboard and falling notes

### Audio
- High-quality piano samples (Salamander Grand Piano)
- 16 velocity layers for realistic dynamics
- Master volume control
- Polyphonic playback

### Themes
- **One Dark** - Dark theme based on Atom's One Dark
- **Latte** - Light theme based on Catppuccin Latte

### Localization
- **English / 简体中文** - Switch with the language chip in the toolbar
- Follows the system/browser language on first launch
- Chord names, tooltips, loading screen and desktop menus are localized too

### File Management
- Open MIDI files via file picker or drag-and-drop
- Multiple file support with easy switching
- Export to MIDI
- Persistent settings (theme, volume, colors, MIDI device)

### PWA Support
- Install as a standalone app on desktop or mobile
- Automatic updates when new versions are available
- Offline support with cached piano samples

## Getting Started

### Prerequisites
- Node.js 18+ (npm is used by the scripts; Bun works too)
- A modern browser with WebMIDI support (Chrome, Edge, Opera)

### Installation

The frontend and the backend are installed separately (no npm workspaces — that
keeps the repo installable on FAT32/exFAT drives, where npm cannot create the
workspace symlinks):

```bash
# Install backend dependencies
npm install

# Install frontend dependencies
npm --prefix client install

# Start development server (API + Vite dev server)
npm run dev
```

The app runs at `http://localhost:5173` (client) with the API server at `http://localhost:3000`.

### Production Build

```bash
# Build both client and server
npm run build

# Start production server
npm run start:prod
```

### Desktop App (Windows `.exe`)

The desktop build wraps the same frontend in Electron and serves it from a
loopback HTTP server (required for WebMIDI, which needs a secure context).

```bash
# Build the client, then package the installer + portable exe
npm run desktop:build

# Only create an unpacked app folder (fast, no installers)
npm run desktop:dir
```

Artifacts land in `release/`:

| File | Description |
| --- | --- |
| `NoteRain-Setup-<version>.exe` | NSIS installer (per-user, custom install dir, desktop/start-menu shortcuts, `.mid` file association) |
| `NoteRain-Portable-<version>.exe` | Single-file portable build — no installation needed |
| `win-unpacked/NoteRain.exe` | Unpacked app folder |

The desktop shell adds a native menu (Open MIDI…, zoom, dev tools, about),
`Ctrl+O` to open a file, drag & drop, and opening `.mid` files from the
command line or "Open with".

### Docker

```bash
# Build and run with Docker
npm run build
docker build -t noterain .
docker run -p 3000:3000 noterain
```

## Usage

1. **Load a MIDI file** - Click "Open MIDI" or drag a .mid file onto the window
2. **Choose a view** - Toggle between falling notes and sheet music
3. **Connect a MIDI keyboard** (optional) - Select your device from the dropdown
4. **Enable Wait Mode** (optional) - Practice by playing along; playback pauses until you hit the right notes
5. **Set a loop** (optional) - Enter start/end measure numbers to practice a section
6. **Press Play** - Watch, listen, and learn!

### Keyboard shortcuts

| Key | Action |
| --- | --- |
| `Space` | Play / pause |
| `←` / `→` | Seek −5 s / +5 s |
| `↑` / `↓` | Speed +5 % / −5 % |
| `K` | Stop |
| `L` | Toggle measure loop |
| `M` | Mute / unmute |
| `Ctrl+O` (desktop) | Open a MIDI file |

Shortcuts are ignored while typing in a form field.

## Tech Stack

- **Frontend**: React 19, TypeScript, Vite
- **Visualization**: PixiJS (falling notes), VexFlow (sheet music)
- **Audio**: Tone.js with Salamander Grand Piano samples
- **MIDI**: WebMIDI API, midi-file parser
- **State**: Zustand with localStorage persistence
- **Backend**: NestJS (serves static files and API)
- **Desktop**: Electron + electron-builder (NSIS installer & portable exe)

## Project Structure

```
noterain/
├── client/                 # React frontend
│   ├── src/
│   │   ├── components/     # UI components
│   │   │   ├── Controls/   # Playback controls, settings panel
│   │   │   ├── FallingNotes/   # PixiJS visualization
│   │   │   ├── PianoKeyboard/  # Interactive keyboard
│   │   │   └── SheetMusic/     # VexFlow notation
│   │   ├── hooks/          # Custom React hooks
│   │   ├── i18n/           # UI strings (English / 简体中文)
│   │   ├── lib/            # MIDI parsing, audio engine, desktop bridge
│   │   ├── stores/         # Zustand state management
│   │   └── types/          # TypeScript types
│   └── public/             # Static assets (piano samples, demo song)
├── desktop/                # Electron shell (main / preload / builder config)
│   ├── build/              # App icons
│   └── scripts/            # Icon generation
├── src/                    # NestJS backend
├── dist/                   # Production build output
└── release/                # Packaged desktop artifacts
```

## Attributions

### Piano Samples
Piano samples from [Salamander Grand Piano V3](https://sfzinstruments.github.io/pianos/salamander) by Alexander Holm - [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/)

### Sheet Music Rendering
Sheet music rendering powered by [VexFlow](https://www.vexflow.com/) - [MIT License](https://github.com/0xfe/vexflow/blob/master/LICENSE)

## License

GPL-3.0
