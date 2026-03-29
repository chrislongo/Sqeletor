# FreakQuencer — Technical Design

High-level architecture for a JUCE-based AUv2 MIDI Effect step sequencer.

---

## Component overview

```
┌──────────────────────────────────────────────────────────────────────┐
│  PluginEditor (UI)                                                   │
│  ┌────────┐  ┌──────────────────────────────────────────┐  ┌──────┐ │
│  │ RATE   │  │  TileGrid — 2 rows × 4 columns           │  │ KEY  │ │
│  │  1/8   │  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │  ├──────┤ │
│  ├────────┤  │  │  C   │ │  E   │ │  G   │ │  B   │   │  │SCALE │ │
│  │ MODE   │  │  └──────┘ └──────┘ └──────┘ └──────┘   │  └──────┘ │
│  │   →    │  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │           │
│  ├────────┤  │  │  D   │ │  F#  │ │  A   │ │  –   │   │           │
│  │  REC   │  │  └──────┘ └──────┘ └──────┘ └──────┘   │           │
│  ├────────┤  └──────────────────────────────────────────┘           │
│  │  REST  │                                                          │
│  └────────┘                                                          │
└─────────┬────────────────────────────────────────────────────────────┘
          │ reads shared state via atomic / lock-free fields
          │ writes user actions (record toggle, mode change, note edit)
┌─────────▼───────────────────────────────────────────────────────────┐
│  PluginProcessor (real-time thread)                                  │
│                                                                      │
│  ┌────────────┐  ┌──────────────┐  ┌─────────────────────┐          │
│  │ Sequencer  │  │ TransportSync│  │ MIDI Router         │          │
│  │ State      │◄─┤ (host tempo, │  │ (input → record,    │          │
│  │            │  │  beat pos,   │  │  playback → output)  │          │
│  │ steps[]    │  │  play/stop)  │  │                     │          │
│  │ stepCount  │  └──────────────┘  └─────────────────────┘          │
│  │ currentIdx │                                                      │
│  │ mode/rate  │                                                      │
│  └────────────┘                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

There are only two source files: `PluginProcessor` and `PluginEditor`. All logic lives in these two classes. No helper libraries, no additional abstraction layers.

---

## Processor — PluginProcessor

The processor runs on the audio thread. It does no audio DSP — `processBlock` reads/writes only the `MidiBuffer`.

### Sequencer state

```cpp
struct Step {
    int noteNumber = -1;  // MIDI note 0–127, or -1 for rest
    int savedNote  = 60;  // last real note; restored when toggling rest off
};

std::array<Step, 8>  steps_;
std::atomic<int>     stepCount_;    // 0–8, number of filled steps
std::atomic<int>     currentStep_;  // index of the step currently playing (shared with editor)
```

- The sequencer initializes with all 8 steps as rests (`noteNumber = -1`) and `stepCount_ = 8`. The user always sees 8 tiles; recording overwrites them from the top.
- `steps_` and `stepCount_` are modified on the audio thread during recording, or via the lock-free note-edit mechanism from the editor.
- `currentStep_` is atomic — written by the processor each time the step advances, read by the editor for highlight.
- `savedNote` preserves the last real pitch when a step is toggled to rest, so it can be restored by Cmd+click.

### APVTS parameters

| Parameter ID | Type | Values | Default | Notes |
|---|---|---|---|---|
| `rate` | Choice | `1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4., 1/8., 1/16., 1/4T, 1/8T, 1/16T` | `1/8` (index 3) | Dotted and triplet variants included |
| `mode` | Choice | `Forward, Reverse, Random` | `Forward` (index 0) | |
| `recording` | Bool | on/off | off | Toggled by Record tile; processor auto-clears on new recording |
| `key` | Choice | `C, C#, D, D#, E, F, F#, G, G#, A, A#, B` | `C` (index 0) | Root note for scale snapping |
| `scale` | Choice | `Off, Major, Minor, Dorian, Phrygian, Lydian, Mixolyd., Locrian` | `Off` (index 0) | Scale for snapping; 0 = off |

Rate, mode, key, and scale are persisted via APVTS. Step data (`steps_`, `stepCount_`) is serialized in `getStateInformation` / `setStateInformation` as raw bytes appended after the APVTS XML block.

### processBlock flow

```
processBlock(buffer, midiMessages):
    outputMidi = copy of all incoming midiMessages   ← always pass through

    apply any pending tile reorder (atomic flag + shadow buffer)
    apply any pending note edit (pendingNoteEditStep_ / pendingNoteEditNote_)

    if recording just started (wasRecording_ == false):
        send note-off for any sounding note
        clear steps_[], reset stepCount to 0
        currentStep_ = kMaxSteps ("none")

    if restPending_ flag set and recording:
        append rest step (noteNumber = -1), increment stepCount

    if recording:
        for each incoming MIDI note-on:
            steps_[stepCount++] = noteNumber
            if stepCount == 8: stop recording
        swap midi ← outputMidi   ← passthrough still delivered
        return

    read AudioPlayHead → ppqPosition, isPlaying

    if not isPlaying:
        send note-off for any sounding note
        reset currentStep_ to kMaxSteps ("none")
        swap midi ← outputMidi
        return

    if stepCount == 0:
        swap midi ← outputMidi
        return

    stepsPerBeat = kStepsPerBeat[rateParam->getIndex()]
    beatInLoop   = fmod(ppq * stepsPerBeat, stepCount)
    rawStep      = floor(beatInLoop)
    playStep     = resolveStep(rawStep, stepCount)   ← applies mode

    if playStep != currentStep_:
        if rawStep != lastRawStep_:    ← genuine step boundary
            send note-off for previous note
            currentStep_ = playStep
            note = steps_[playStep].noteNumber
            if note >= 0:
                note = snapToScale(note, key, scale)
                outputMidi += noteOn(note, velocity=100)
        else:                          ← mode changed mid-step, no MIDI
            currentStep_ = playStep

    lastRawStep_ = rawStep
    swap midi ← outputMidi
```

### Scale snapping

`snapToScale(noteNum, key, scaleIdx)` is a static method callable from both the processor and editor:

- If `scaleIdx == 0` (Off) or `noteNum < 0` (rest), returns the note unchanged.
- Finds the pitch class in the selected scale with the smallest circular chromatic distance to the input note.
- If the note is already in the scale, returns it unchanged.
- Otherwise snaps to the nearest MIDI note number with the target pitch class (checking ±1 octave).

The snap is applied at emit time, not stored — the raw recorded note is preserved in `steps_[]`.

### Step advancement by mode

- **Forward**: `rawStep = floor(fmod(ppq * stepsPerBeat, stepCount))`
- **Reverse**: `playStep = stepCount - 1 - rawStep`
- **Random (shuffle)**: Pre-generate a permutation of `[0..stepCount-1]` (Fisher-Yates). Walk through it using `rawStep` as the index. Regenerate when the cycle wraps (`rawStep == 0` after a non-zero step). This ensures every step plays exactly once per cycle.

### Beat-position tracking

The processor reads `ppqPosition` from the host's `AudioPlayHead` on every `processBlock` call:

```
stepsPerBeat = rate divisor (e.g. 1/8 → 2 steps per beat)
beatInLoop   = fmod(ppqPosition * stepsPerBeat, stepCount)
rawStep      = floor(beatInLoop)
```

This keeps the sequencer perfectly locked to the host — scrubbing, looping, and cycle mode all work correctly because position is always derived from the host, never from an internal counter.

### Recording

Recording is a mode flag. While active:
- Incoming MIDI note-on events are captured into `steps_[]` and also forwarded to the instrument via passthrough.
- Recording ends when `stepCount_ == 8` or the user toggles recording off.
- Starting a new recording clears `steps_[]` and resets `stepCount_` to 0.
- The Rest button sets `atomic<bool> restPending_`; the processor appends a rest step (`noteNumber = -1`) on the next `processBlock` call.
- Recording works regardless of transport state.

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

The editor uses a `juce::Timer` (60 Hz) to poll `currentStep_` and `stepCount_` from the processor. A repaint is triggered when either value changes. An additional repaint is forced for ~150ms after each step change to drive the LED flash decay animation.

### Tile rendering

Each tile is a rectangle in the 2×4 grid. The editor iterates over `steps_[0..stepCount-1]` and draws:

- **Filled note tile**: dark background (`#242432`) + colored note name text (pitch-class color). In the bottom-right corner, a small octave number.
- **Active (playing) note tile**: pitch-class color as background fill; white text; on step change a white flash bursts at full brightness and decays to the pitch color over 150ms.
- **Rest tile**: dark background + dimmed en-dash (–) in the tile's associated pitch-class color (35% alpha).
- **Empty slot** (index >= stepCount): very faint outline only.

The note displayed (and colored) is the scale-snapped version — tiles show what will actually play.

**Pitch-class color palette** (12 neon colors):
| Pitch | Color | Hex |
|---|---|---|
| C | hot pink | `#e8347a` |
| C# | rose-magenta | `#cc3aaa` |
| D | violet | `#b43de8` |
| D# | indigo | `#6b3de8` |
| E | blue | `#3d7de8` |
| F | sky | `#35c4e8` |
| F# | teal | `#35e8c4` |
| G | green | `#35e878` |
| G# | lime | `#a8e835` |
| A | yellow | `#e8d435` |
| A# | orange | `#e87835` |
| B | red | `#e83535` |

Note name rendering: `juce::MidiMessage::getMidiNoteName(displayNote, true, false, 4)` — `includeOctaveNumber = false`.

### Controls

All controls are drawn directly inside `PluginEditor::paint()` as custom rectangles — no JUCE `Component` subclasses. Click and drag handling is in `mouseDown()`, `mouseDrag()`, and `mouseUp()`.

**Left column (4 tiles):**
- **Rate tile**: click cycles through 12 rate options; vertical drag scrubs (32px/step). Displays current name.
- **Mode tile**: click cycles Forward → Reverse → Random; vertical drag scrubs. Always shown active (pink). Displays directional symbol (→ / ← / ⇄).
- **Record tile**: click toggles `recording` parameter. Shown active (pink) while recording.
- **Rest tile**: click sets `restPending_` flag (only effective during recording). Displays —.

**Right column (2 tiles):**
- **Key tile**: click cycles through 12 root notes; vertical drag scrubs. Highlighted (pink) when a scale is active.
- **Scale tile**: click cycles Off/Major/Minor/Dorian/Phrygian/Lydian/Mixolydian/Locrian; vertical drag scrubs. Highlighted when active.

### Note tile interaction

Two gestures are available on filled note tiles (outside recording mode):

**2-axis drag (pitch + octave edit):**
- Mouse-down on a tile begins a drag; the tile starts showing a live preview.
- Drag left/right: changes the pitch class in chromatic steps (one step per 20px, wraps through 12).
- Drag up/down: changes the octave (one octave per 40px up, clamped 0–9).
- On mouse-up: commits via `pendingNoteEditStep_` / `pendingNoteEditNote_` atomics.

**Cmd+click (rest toggle):**
- If the step is a note: sets `noteNumber = -1` (rest), saves the current pitch in `savedNote`.
- If the step is a rest: restores `savedNote` as the active note.
- Committed immediately via the pending note edit mechanism.

### Layout

```cpp
namespace {
    constexpr int kPadding      = 14;   // panel edge padding
    constexpr int kGap          = 8;    // gap between all tiles
    constexpr int kCtrlColWidth = 68;   // left and right control column width
    constexpr int kTileW        = 120;  // note tile width
    constexpr int kTileH        = 120;  // note tile height (square)
    constexpr int kGridCols     = 4;
    constexpr int kGridRows     = 2;
    constexpr int kNumCtrl      = 4;    // tiles in left column
    constexpr int kNumScaleCtrl = 2;    // tiles in right column

    constexpr int kGridW  = kGridCols * kTileW + (kGridCols - 1) * kGap;
    constexpr int kPanelW = kPadding * 2 + kCtrlColWidth + kGap + kGridW + kGap + kCtrlColWidth;
    constexpr int kPanelH = kPadding * 2 + kGridRows * kTileH + (kGridRows - 1) * kGap;
}
```

Left control column tiles split `kPanelH` evenly across `kNumCtrl` tiles. Right column tiles split across `kNumScaleCtrl` tiles (taller tiles).

---

## Thread safety model

There are only two threads: the audio thread (processor) and the message thread (editor).

| Data | Written by | Read by | Mechanism |
|---|---|---|---|
| `currentStep_` | Audio thread | Message thread | `std::atomic<int>` |
| `stepCount_` | Audio thread (recording) | Message thread | `std::atomic<int>` |
| `steps_[]` | Audio thread (recording) | Message thread (display) | Atomic stepCount acts as a release fence — editor only reads indices < stepCount |
| APVTS params | Message thread (UI) | Audio thread | JUCE APVTS (lock-free by design) |
| Tile reorder | Message thread | Audio thread | Lock-free swap via atomic flag + shadow buffer |
| Note edit | Message thread | Audio thread | `pendingNoteEditStep_` / `pendingNoteEditNote_` atomics; processor applies on next block |
| Rest pending | Message thread | Audio thread | `std::atomic<bool> restPending_` |

### MIDI passthrough implementation

`processBlock` initializes `outputMidi` by copying all incoming events (`outputMidi.addEvents(midi, 0, -1, 0)`) before any other logic. All subsequent code paths do `midi.swapWith(outputMidi)` at exit, so passthrough is guaranteed in every state.

### Lock-free note edit

When the user finishes a drag or Cmd+click:
1. Editor writes the new note number to `pendingNoteEditNote_` and the step index to `pendingNoteEditStep_` (which acts as the commit flag, set last).
2. On the next `processBlock`, the processor atomically exchanges `pendingNoteEditStep_` with `-1` — if the result is ≥ 0, it applies the edit to `steps_[]`.

---

## State persistence

`getStateInformation` serializes:
1. APVTS parameter state (XML via `copyState().toXmlString()`)
2. Step data as a binary block: `stepCount` (int32) followed by `stepCount` × `noteNumber` (int32 each)

`setStateInformation` deserializes in the same order. If the binary block is missing or corrupt, the sequencer starts empty.

---

## MIDI FX specifics

### AU MIDI Effect type

As an `aumi` type AU, the plugin receives MIDI input and produces MIDI output. It does not process audio. In JUCE:
- `isMidiEffect()` returns `true`
- `acceptsMidi()` returns `true`
- `producesMidi()` returns `true`
- `processBlock` receives a silent audio buffer (ignored) and a `MidiBuffer`

### Autonomous playback

Most MIDI FX plugins only transform or filter incoming MIDI. FreakQuencer generates MIDI output from its internal sequencer state regardless of whether any MIDI input is arriving. The AU host calls `processBlock` continuously while the transport is running, even with no MIDI input — the processor uses the host's beat position to decide when to emit notes.

---

## File structure

```
FreakQuencer/
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
│   ├── mock-1.png          ← initial layout sketch
│   └── mock-2.html         ← interactive placeholder mock
└── src/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```
