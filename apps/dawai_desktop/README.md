# dawai_desktop (Phase 8 Stub)

Minimal DawAI desktop stub proving shared engine integration (`aifr3d_core`) without JUCE plugin hosting.

## Build

From repo root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel --target dawai_desktop_stub
```

## Run

Session dashboard mode (loads VST-exported session JSON bundle):

```bash
./build/apps/dawai_desktop/dawai_desktop_stub --session /path/to/session_folder
```

Expected files in session folder:
- `analysis.json`
- `issues.json`

Offline analysis mode (optional core analysis path):

```bash
./build/apps/dawai_desktop/dawai_desktop_stub --analyze /path/to/audio.wav
```

## Scope (intentional stub)
- Text/CLI dashboard only (no heavy GUI framework)
- Displays overall score + key metrics + top issue preview
- Performs light required-field checks for session JSON
- No plugin hosting, no DAW transport/control integration

