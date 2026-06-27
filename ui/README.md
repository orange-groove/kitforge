# KitForge WebView UI

KitForge’s main product UI is a **React + TypeScript** app rendered inside a **JUCE `WebBrowserComponent`**. Audio, MIDI, file I/O, catalog/install, and kit serialization remain in C++.

## Folder structure

```
ui/
  package.json
  vite.config.ts
  index.html
  src/
    main.tsx
    App.tsx
    theme.ts
    types/kit.ts
    bridge/
      kitforgeBridge.ts   # React ↔ JUCE JSON messages
      juceShim.ts         # JUCE native function helper
    mock/mockKit.ts       # Browser dev fallback
    components/
      layout/             # AppShell, TopBar, SidePanel
      kit/                # DrumCanvas, DrumPieceNode, KitInspector
      controls/           # AudioKnob, AudioSlider wrappers (Cutoff)
      library/            # Library browser panels
      ai/                 # AIBuilderPanel
      settings/           # SettingsPanel (stub)

Source/UI/
  WebViewBridge.h/cpp     # C++ message dispatch + kit/catalog push

Source/PluginEditor.*     # WebView host (replaces tabbed JUCE UI)
```

## Licensing note

**Cutoff AudioUI** (`@cutoff/audio-ui-react`) is dual-licensed **GPL-3.0 / Commercial**. Verify licensing before any commercial release.

## Development workflow

### Terminal 1 — React dev server

```bash
cd ui
npm install
npm run dev
```

Runs Vite at `http://localhost:5173`.

### Terminal 2 — JUCE with dev server

Reconfigure CMake with the dev-server flag, then build:

```bash
cmake --preset ninja-debug -DKITFORGE_USE_DEV_SERVER=ON
cmake --build build-ninja --target KitForge_Standalone
```

The plugin editor loads `http://localhost:5173` instead of bundled `ui/dist/`.

### Production UI build

CMake runs `npm install && npm run build` automatically before compiling (enabled by default via `KITFORGE_BUILD_UI`):

```bash
cmake --preset ninja-debug
cmake --build build-ninja --target KitForge_Standalone
```

To skip the UI step (e.g. when using the Vite dev-server preset): `-DKITFORGE_BUILD_UI=OFF`

Manual UI-only build:

```bash
cd ui && npm install && npm run build
```

Output: `ui/dist/index.html` (+ assets). The Standalone app bundle copies `ui/dist/` into `Contents/Resources/ui/dist/`.

## How JUCE loads the WebView

`KitForgeAudioProcessorEditor` creates a `WebBrowserComponent` with:

- `withNativeIntegrationEnabled()` — exposes `window.__JUCE__.backend`
- `withNativeFunction("kitforgeMessage", …)` — receives JSON from React
- `withResourceProvider(…)` — serves `ui/dist/` at `juce://juce.backend/` (macOS/Linux)

Load URL:

| Mode | URL |
|------|-----|
| Dev server | `http://localhost:5173` |
| Production | `WebBrowserComponent::getResourceProviderRoot()` |

If `ui/dist/index.html` is missing, a fallback label explains how to build the UI.

## Message bridge

### React → C++

Send JSON via `kitforgeBridge.*` helpers → native function `kitforgeMessage`:

```json
{ "type": "triggerPiece", "pieceId": "snare" }
{ "type": "movePiece", "pieceId": "rackTom1", "x": 0.48, "y": 0.36 }
{ "type": "saveKit" }
{ "type": "aiBuildKit", "prompt": "Build me a tight modern metal kit" }
```

On startup React sends `{ "type": "ready" }`. C++ responds with full state.

### C++ → React

`WebViewBridge::sendToWeb()` emits `kitforgeFromNative` events:

```json
{ "type": "kitState", "kit": { "version": 2, "kitName": "…", "pieces": […] } }
{ "type": "catalogState", "libraries": [], "installed": [] }
{ "type": "error", "message": "…" }
```

**C++ `KitModel` is the source of truth.** React mirrors `kitState` and sends actions; C++ updates the model and pushes a fresh `kitState`.

All WebView callbacks run on the **message thread**; audio work stays in `DrumSamplerEngine` / `processBlock`.

## Drum canvas

- Pieces use **normalized 0–1** coordinates in JSON (C++ converts to/from pixel layout internally).
- Drums render as circles; cymbals as flattened ovals.
- Click triggers `triggerPiece`; right-click opens context menu (learn MIDI, assign sample, rename, delete).
- **Edit Layout** enables drag; moves are debounced with `requestAnimationFrame` before sending `movePiece`.

## Stubbed / partial

| Area | Status |
|------|--------|
| Settings panel in React | Stub — use standalone **Settings** toolbar button for OpenAI key |
| Library install preview | Auto-confirms SFZ preview (no mapping modal in WebView yet) |
| Mixer window | Still native JUCE (`MixerPanel`) — not yet in React |
| Rename/delete/add piece types | Basic bridge support; no full kit editor toolbar in React |
| Windows WebView | Requires WebView2 (`NEEDS_WEBVIEW2` in CMake) |

## Platform notes

- **macOS**: WKWebView — best supported dev target.
- **Windows**: WebView2 (Edge Chromium) — set temp user-data folder if needed in plugins.
- **Linux**: WebKitGTK — resource provider available.
- **Browser-only**: `npm run dev` uses mock kit data when `window.__JUCE__` is absent.

## UI stack

- **Layout / panels**: Chakra UI v2
- **Audio controls**: Cutoff `@cutoff/audio-ui-react` via `AudioKnob` / `AudioSlider` wrappers
- **Build**: Vite + React 18 + TypeScript (not Next.js)
