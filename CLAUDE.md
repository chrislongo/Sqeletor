# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

FreakQuencer is a **Logic Pro AUv2 MIDI Effect plugin** — a step sequencer that lives in Logic's MIDI FX slot. The user records up to 8 notes by playing a MIDI keyboard, and the plugin loops them back in sync with Logic's transport.

The core differentiator: it's an _autonomous_ MIDI FX (no held input needed), with keyboard-entry recording (play it in, don't click it in), and a tile-based UI where large note names are the primary visual element. See [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md) for full functional spec and [docs/IDEA.md](docs/IDEA.md) for design philosophy.

## Plugin identity

| Field | Value |
|---|---|
| Product name | FreakQuencer |
| AU type | `aumi` (MIDI processor) |
| Manufacturer code | `Cvda` |
| Plugin code | `Frqn` |
| Bundle ID | `com.CorvidAudio.FreakQuencer` |
| CMake target | `FreakQuencer` |

## Toolchain

Identical to the rest of the Corvid-DSP workspace:

- **cmake**: `/opt/homebrew/bin/cmake` — Xcode is **not** installed; always use `-G Ninja`
- **Compiler**: `xcrun -f clang` / `xcrun -f clang++` (Apple Clang via Xcode CLI tools)
- **JUCE**: `/Users/chris/src/github/JUCE` (added via `add_subdirectory`)
- No eurorack dependency — FreakQuencer is pure MIDI, no audio DSP

## Build commands

Run from the repo root (FreakQuencer lives as a standalone repo, not inside Corvid-DSP):

```bash
# Configure (once, or after CMakeLists.txt changes)
/opt/homebrew/bin/cmake -B build -G Ninja \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=$(xcrun -f clang) \
    -DCMAKE_CXX_COMPILER=$(xcrun -f clang++)

# Build
/opt/homebrew/bin/cmake --build build --config Release

# Install, sign, validate
rm -rf ~/Library/Audio/Plug-Ins/Components/FreakQuencer.component
cp -R build/FreakQuencer_artefacts/Release/AU/FreakQuencer.component ~/Library/Audio/Plug-Ins/Components/
codesign --force --sign "-" ~/Library/Audio/Plug-Ins/Components/FreakQuencer.component
auval -v aumi Frqn Cvda
```

## Architecture

### Source layout

```
src/
  PluginProcessor.cpp / PluginProcessor.h   — MIDI logic, sequencer state
  PluginEditor.cpp   / PluginEditor.h       — tile UI
```

### Processor responsibilities

- Sequencer state: recorded steps, current step index, playback mode, rate
- Host sync: read `AudioPlayHead` for tempo, beat position, and play/stop state
- MIDI I/O: consume incoming note-ons during recording; emit note-on/off during playback
- No audio processing — `processBlock` only touches the MIDI buffer

### Editor responsibilities

- Horizontal row of up to 8 note tiles (the primary UI element)
- Rate selector, playback mode buttons, Record button, Rest button
- Highlight the currently playing tile; pulse Record button while recording

## JUCE conventions (inherited from Corvid-DSP workspace)

- `juce_generate_juce_header(<Target>)` is required
- Use `juce::Font(juce::FontOptions().withHeight(n))` — the `Font(float)` constructor is deprecated
- `AudioProcessorEditor` already has a `processor` member; don't shadow it in subclasses
- Index `std::array` with `static_cast<size_t>()` to avoid `-Wsign-conversion`
- APVTS lives in the processor (public); editor uses `SliderAttachment` / `ButtonAttachment`

## Testing

No automated tests. `auval -v aumi Frqn Cvda` is the primary correctness check.

## Design language

FreakQuencer intentionally departs from the silver-grey Corvid Audio house style to be more fun and vibrant. See [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md) for the full color spec.
