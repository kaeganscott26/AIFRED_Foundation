# AIFRED (Electron + Android WebView)

AIFRED now uses the official OpenAI API for chat and speech, with optional local routing and a persistent Memory Vault.

## Backend and models

- Chat endpoint: `POST https://api.openai.com/v1/chat/completions`
- TTS endpoint: `POST https://api.openai.com/v1/audio/speech`
- Supported chat models:
  - `gpt-4o-mini`
  - `gpt-4o`
  - `gpt-4.1-mini`
  - `gpt-4.1`
- Local function-calling tool schema is preserved.
- `Legacy / anonymous` mode is available behind a toggle.

## API key setup

Desktop (Electron):

```bash
export OPENAI_API_KEY="sk-..."
npm start
```

If no key is configured, the UI shows: `OpenAI API key not configured.`

Android/WebView:

- Provide `window.OPENAI_API_KEY` from host/app code if cloud mode is used.

## Cognition Layer

New renderer cognition modules:

- `src/renderer/cognition/personality.js`
- `src/renderer/cognition/intent.js`
- `src/renderer/cognition/profile_store.js`
- `src/renderer/cognition/router.js`
- `src/renderer/cognition/memory_score.js`

Capabilities:

- Adaptive personality vector tuning (persistent)
- Intent weighting + trace output
- Long-term profile persistence + export/reset
- Hybrid route selection (`local` / `cloud` / `legacy`) with privacy toggles
- Reinforcement memory scoring for vault recall

## Memory Vault

Desktop storage:

- `userData/vault/index.json`
- `userData/vault/files/<sha256>.<ext>`
- `userData/vault/summaries/<id>.json`
- `userData/vault/features/<id>.json`
- `userData/vault/profile.json`

Android/WebView storage:

- Vault metadata in `localStorage`
- Small file payloads (<= 5MB) as base64
- Large file behavior: metadata-only + warning (`Android large file storage not supported without native file bridge`)

Vault features:

- Choose file(s) and ingest
- Search by filename, tags, summary text
- Attach item to chat context
- Pin / Hide / Forget actions
- Auto summary + tags generation via OpenAI (when key is available)
- Audio/video feature JSON stubs saved to `vault/features/`

## Building a reference pool

1. Import files with `Choose File(s)` + `Ingest into Vault`.
2. Use `scan_folder` tool (desktop, with explicit confirmation) to bulk-ingest a folder.
3. Verify items appear in Vault search results and have scores/summaries.

## Privacy and routing

Routing controls in UI:

- `Local mode`
- `Prefer local for private data`
- `Allow cloud for private data` (default off)
- `Legacy / anonymous`
- `Test local endpoint`

Function Trace includes:

- `[intent]` weights
- `[route]` decisions and reasons
- `[memory]` retrieval scoring details

## Run and checks

```bash
npm install
npm run check
npm start
```

## Quick sanity tests

1. Enable `Adaptive personality`, chat with style instructions (`shorter`, `more detailed`) and verify trait readout shifts.
2. Import a file, confirm it appears in vault search with score metadata.
3. Enable `Local mode` with an unreachable endpoint and verify fallback route in trace.
4. On Android, pick a small file and confirm metadata ingest.
