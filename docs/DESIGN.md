# Sqeletor — Technical Design

High-level architecture for a JUCE-based AUv2 MIDI Effect step sequencer.

---

## Component overview

```
┌─────────────────────────────────────────────────────────────┐
│  PluginEditor (UI)                                          │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  TileStrip — 8 tile slots, horizontal row            │   │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                │   │
│  │  │  C4  │ │  E4  │ │  —   │ │  G4  │ ...            │   │
│  │  └──────┘ └──────┘ └──────┘ └──────┘                │   │
│  └──────────────────────────────────────────────────────┘   │
│  [ Rate ▾ ]   [ → ] [ ← ] [ ⤮ ]   [ ● REC ] [ REST ]     │
└─────────┬───────────────────────────────────────────────────┘
          │ reads shared state via atomic / lock-free fields
          │ writes user actions (record toggle, mode change)
┌─────────▼───────────────────────────────────────────────────┐
│  PluginProcessor (real-time thread)                          │
│                                                              │
│  ┌────────────┐  ┌──────────────┐  ┌─────────────────────┐  │
│  │ Sequencer  │  │ TransportSync│  │ MIDI Router         │  │
│  │ State      │◄─┤ (host tempo, │  │ (input → record,    │  │
│  │            │  │  beat pos,   │  │  playback → output)  │  │
│  │ steps[]    │  │  play/stop)  │  │                     │  │
│  │ stepCount  │  └──────────────┘  └─────────────────────┘  │
│  │ currentIdx │                                              │
│  │ mode/rate  │                                              │
│  └────────────┘                                              │
└──────────────────────────────────────────────────────────────┘
```

There are only two source files: `PluginProcessor` and `PluginEditor`. All logic lives in these two classes. No helper libraries, no additional abstraction layers.

---

## Processor — PluginProcessor

The processor runs on the audio thread. It does no audio DSP — `processBlock` reads/writes only the `MidiBuffer`.

### Sequencer state

```cpp
struct Step {
    int  noteNumber;   // MIDI note 0–127, or -1 for rest
};

std::array<Step, 8>  steps_;
int                  stepCount_;    // 0–8, number of filled steps
std::atomic<int>     currentStep_;  // index of the step currently playing (shared with editor)
```

- `steps_` and `stepCount_` are modified only by the processor on the audio thread (during recording) or by the editor via a lock-free update mechanism (tile reorder in P1).
- `currentStep_` is atomic — written by the processor each time the step advances, read by the editor for highlight.

### APVTS parameters

| Parameter ID | Type | Values | Notes |
|---|---|---|---|
| `rate` | Choice | `1/1, 1/2, 1/4, 1/8, 1/16, 1/32` | Index 0–5 |
| `mode` | Choice | `Forward, Reverse, Random` | Index 0–2 |
| `recording` | Bool | on/off | Toggled by Record button |

Rate and mode are persisted via APVTS and saved/restored with the plugin state. The step data (`steps_`, `stepCount_`) is serialized in `getStateInformation` / `setStateInformation` as raw bytes appended after the APVTS XML block.

### processBlock flow

```
processBlock(buffer, midiMessages):
    read AudioPlayHead → tempo, ppqPosition, isPlaying

    if not isPlaying:
        send note-off for any sounding note
        reset currentStep to start position (based on mode)
        return

    if recording:
        for each incoming MIDI event:
            if note-on and stepCount < 8:
                steps_[stepCount_++] = { noteNumber }
                if stepCount_ == 8:
                    set recording = false
        clear outgoing MIDI
        return

    if stepCount == 0:
        return

    calculate stepDuration in samples from tempo + rate
    calculate current beat position within the loop

    if step boundary crossed since last processBlock call:
        send note-off for previous step (unless it was a rest)
        advance currentStep (forward / reverse / random logic)
        if current step is not a rest:
            send note-on (note, velocity=100)
            schedule note-off at 80% of stepDuration
```

### Step advancement by mode

- **Forward**: `currentStep = (currentStep + 1) % stepCount`
- **Reverse**: `currentStep = (currentStep - 1 + stepCount) % stepCount`
- **Random (shuffle)**: Pre-generate a permutation of `[0..stepCount-1]`. Walk through it sequentially. Regenerate when the cycle completes. This ensures every step plays exactly once per cycle.

### Beat-position tracking

The processor does not maintain a free-running sample counter. Instead, it reads `ppqPosition` from the host's `AudioPlayHead` on every `processBlock` call and derives which step should be active:

```
stepsPerBeat = rate divisor (e.g. 1/8 → 2 steps per beat)
beatInLoop   = fmod(ppqPosition * stepsPerBeat, stepCount)
stepIndex    = floor(beatInLoop)
```

This keeps the sequencer perfectly locked to the host — scrubbing, looping, and cycle mode all work correctly because position is always derived from the host, never from an internal counter.

The gate-off point is derived the same way: when `fractional part of beatInLoop` crosses 0.8 within the current buffer, emit the note-off.

### Recording

Recording is a mode flag. While active:
- Incoming MIDI note-on events are consumed and appended to `steps_[]`.
- No MIDI is passed through or generated.
- Recording ends when `stepCount_ == 8` or the user toggles recording off.
- Starting a new recording clears `steps_[]` and resets `stepCount_` to 0.
- The Rest button (handled via a parameter or direct message from the editor) appends a step with `noteNumber = -1`.

### Note-off safety

The processor tracks which note is currently sounding (`lastSentNote_`). A note-off is sent:
- Before sending the next step's note-on
- When transport stops
- When recording starts
- When the plugin is bypassed or removed (`releaseResources`)

This prevents stuck notes in all cases.

---

## Editor — PluginEditor

The editor runs on the message thread. It draws the tile UI and handles user interaction.

### Timer-driven repaint

The editor uses a `juce::Timer` (30–60 Hz) to poll `currentStep_` from the processor and repaint the tile strip. This is the standard JUCE pattern — the editor never touches the audio thread's data structures directly, only reads atomics.

### Tile rendering

Each tile is a rectangle in the tile strip area. The editor iterates over `steps_[0..stepCount-1]` and draws:

- **Filled tile**: solid color rectangle + note name text (e.g. "C4", "F#3"). At P0, all tiles share a single accent color. At P1, each tile gets a pitch-class color.
- **Rest tile**: dark grey rectangle + dash/rest symbol.
- **Empty slot** (index >= stepCount): very dark outline only.
- **Active tile** (index == currentStep): brighter variant of the tile's color.

Note name rendering uses `juce::MidiMessage::getMidiNoteName(noteNumber, true, true, octaveMiddleC)`.

### Controls

All controls are wired to APVTS parameters via attachments:

- **Rate selector**: `juce::ComboBox` + `ComboBoxAttachment` bound to `rate`.
- **Mode buttons**: three `juce::ToggleButton`s in a radio group + custom `ButtonAttachment`s bound to `mode`. Only one is active at a time.
- **Record button**: `juce::ToggleButton` + `ButtonAttachment` bound to `recording`. Custom paint gives it a pulsing glow while active.
- **Rest button**: plain `juce::TextButton`. On click, sends a message to the processor to append a rest step. Enabled only while recording and `stepCount < 8`.

### Layout

All layout constants are in an anonymous namespace at the top of `PluginEditor.cpp`:

```cpp
namespace {
    constexpr int kPanelWidth  = 600;
    constexpr int kPanelHeight = 200;
    constexpr int kTileMargin  = 6;
    constexpr int kTileY       = 50;
    constexpr int kTileHeight  = 90;
    // tile width is computed: (panelWidth - margins) / 8
}
```

---

## Thread safety model

There are only two threads to worry about: the audio thread (processor) and the message thread (editor).

| Data | Written by | Read by | Mechanism |
|---|---|---|---|
| `currentStep_` | Audio thread | Message thread | `std::atomic<int>` |
| `stepCount_` | Audio thread (recording) | Message thread | `std::atomic<int>` |
| `steps_[]` | Audio thread (recording) | Message thread (display) | Atomic stepCount acts as a release fence — editor only reads indices < stepCount, processor only writes at index == stepCount |
| APVTS params | Message thread (UI) | Audio thread | JUCE APVTS (lock-free by design) |
| Tile reorder (P1) | Message thread | Audio thread | Lock-free swap via atomic flag + shadow buffer (see below) |

### P1: Lock-free tile reorder

When the user finishes a Shift+drag reorder, the editor writes the new step order into a shadow array and sets an atomic flag. On the next `processBlock`, the processor detects the flag, copies the shadow array into `steps_[]`, and clears the flag. This avoids any locks on the audio thread.

---

## State persistence

`getStateInformation` serializes:
1. APVTS parameter state (XML via `copyState().createXml()`)
2. Step data appended as a binary block: `stepCount` (int32) followed by `stepCount` × `noteNumber` (int32 each)

`setStateInformation` deserializes in the same order. If the binary block is missing or corrupt, the sequencer starts empty (no steps recorded).

---

## MIDI FX specifics

### AU MIDI Effect type

As an `aumi` type AU, the plugin receives MIDI input and produces MIDI output. It does not process audio. In JUCE:
- `isMidiEffect()` returns `true`
- `acceptsMidi()` returns `true`
- `producesMidi()` returns `true`
- `processBlock` receives a silent audio buffer (ignored) and a `MidiBuffer`

### Autonomous playback

Most MIDI FX plugins only transform or filter incoming MIDI. Sqeletor generates MIDI output from its internal sequencer state regardless of whether any MIDI input is arriving. This works because the AU host calls `processBlock` continuously while the transport is running, even with no MIDI input — the processor uses the host's beat position to decide when to emit notes.

### Rest button communication

The Rest button lives in the editor (message thread) but needs to append a step on the audio thread. This uses a single-slot lock-free FIFO (or `std::atomic<bool> restPending_`). The editor sets the flag; the processor checks it at the top of `processBlock` and appends a rest step if recording is active.

---

## File structure (P0)

```
Sqeletor/
├── CMakeLists.txt
├── CLAUDE.md
├── README.md
├── LICENSE
├── docs/
│   ├── IDEA.md
│   ├── REQUIREMENTS.md
│   ├── RESEARCH.md
│   └── DESIGN.md          ← this file
├── mocks/
│   └── whiteboard.jpeg
└── src/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```
