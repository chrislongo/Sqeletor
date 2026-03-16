# Sqeletor — Product Requirements

## Differentiation

Sqeletor occupies a gap that no existing product fills:

**vs. Logic's native Step Sequencer** — Logic's sequencer is region-based: you pre-program patterns in the arrangement and commit before you hear them on a synth. Sqeletor lives in the MIDI FX slot as a live insert — drop it on any instrument track or external MIDI track, hit record, play notes, and the loop plays immediately. It works with hardware synths the same way.

**vs. complex software sequencers (Stepic, Thesys)** — These are powerful but require reading a manual before making music. Sqeletor's recording model is a hardware looper: hit record, play up to 8 notes, done. No menus, no note pickers, no mode switching to get started.

**vs. hardware sequencers (Arturia KeyStep, BeatStep Pro)** — Hardware sequencers require physical hardware and don't integrate with Logic's transport and MIDI FX slot. Sqeletor brings the same keyboard-entry workflow (play notes → instant loop) into Logic natively, for free, sequencing any AU instrument or connected hardware.

**vs. generative tools (Riffer, Stochas, Harmony Bloom)** — These generate or sculpt patterns; you don't compose a specific melody. Sqeletor is deterministic: you play exactly what you want to hear, and it plays it back.

**The specific combination nobody else has:**
- Logic AU MIDI FX (live insert, not region-based)
- MIDI keyboard recording (play it in, don't click it in)
- Autonomous playback (no held input needed)
- Works with software instruments and hardware equally
- Free and open source

---

## Goal

A Logic Pro MIDI FX step sequencer that feels like a built-in feature Logic never shipped. Open it, record a phrase, hear it loop in under 30 seconds. Free and open source.

---

## Design tenets

- **Simplicity first** — do a few things really well, not a million things. Every feature proposal should be weighed against this.
- **Fast** — feels like a real instrument. Using it requires almost no thought. From zero to looping in under 30 seconds.
- **Rewards experimentation** — happy accidents are a feature, not a bug. Random mode and future generative features exist to surprise the user.
- **The UI is the product** — the tile row is the visual identity. It ships at P0, not as a later polish pass. If the tiles aren't fun to look at, the plugin isn't done.
- **Free and open source** — always.

---

## Plugin format

- **Type:** AUv2 MIDI Effect — Logic MIDI FX slot
- **Host:** Logic Pro (primary); any AU MIDI FX host (secondary)
- **Autonomous:** Plays back without held MIDI input
- **Host sync:** Tempo and transport from host (start/stop/position)

### Supported track types in Logic

- **Software instrument tracks** — sequences any Logic or third-party AU instrument
- **External MIDI tracks** — sequences hardware synths, drum machines, Eurorack MIDI-CV, etc. via any connected MIDI port/channel; no IAC bus or extra routing needed

---

## Phased delivery

### P0 — MVP (shipped)

The tile UI, dark neon aesthetic, per-pitch colors, LED flash, scale snapping, and 2-axis note editing all shipped at P0. The visual identity is complete.

### P1 — Polish and additional modes

Additional playback modes, per-step velocity/gate, and any remaining quality-of-life improvements.

### P2 — Extended features

Scale-population tools, variable step counts, and generative options.

---

## P0 Requirements (shipped)

### Recording

- Starts with 8 rest steps (all slots filled with rests); the user records over them
- User hits **Record**, then plays notes on a MIDI keyboard; each note-on fills the next step in order
- **Rests** are entered via a dedicated Rest button (tapped instead of playing a note)
- Recording stops automatically when step 8 is filled, or manually when the user hits Record again
- Re-recording replaces the entire sequence (all steps reset to rests, count resets to 0)
- Duplicate pitches supported — each tap = one step regardless of pitch

### Playback

- Plays back the recorded sequence in a loop, locked to host tempo and transport
- Follows host play/stop; resets to step 1 on stop
- No sequencer output when host is stopped (MIDI passthrough still active)

### MIDI passthrough

- All incoming MIDI always passes through to the instrument — the plugin never silences the input stream
- During recording, note-ons are captured into the sequence and also forwarded to the instrument (the player hears what they're recording)
- During sequencer playback, both the sequencer's notes and any live MIDI input are forwarded

### Rate

- Playback rate relative to host tempo
- Options: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, plus dotted (1/4., 1/8., 1/16.) and triplet (1/4T, 1/8T, 1/16T) variants
- Default: 1/8

### Playback modes

- **Forward** — steps 1→N, loop
- **Reverse** — steps N→1, loop
- **Random** — shuffle/permutation (each step plays once per cycle before repeating; not pure random)

### Scale snapping

- **Key** selector: root note C through B (12 options)
- **Scale** selector: Off, Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian
- When a scale is active, every note emitted is snapped to the nearest scale degree at playback time; the stored MIDI note is unchanged
- Scale snap is also applied visually — tiles display the snapped note name and color
- When scale is Off, notes play as recorded

### MIDI output

- Emits Note On / Note Off per step
- Rest steps emit nothing
- Velocity: fixed (100)
- Gate: one full step duration (note-off fires at the next step boundary)

### UI — Layout

- **Left column (~68px):** 4 control tiles stacked vertically — Rate, Mode, Record, Rest
- **Main area:** 8 note tiles in a 2-row × 4-column grid (120×120px square tiles, 8px gaps)
- **Right column (~68px):** 2 scale control tiles stacked vertically — Key, Scale
- Steps fill left-to-right, top-to-bottom (slot 1 = top-left, slot 4 = top-right, slot 5 = bottom-left, slot 8 = bottom-right)
- Panel sizes to content; no fixed dimensions

### UI — Note tiles

- Each filled tile displays the note name in very large bold text (e.g. "C", "F#") — no octave in the main label
- Octave number shown in small text in the bottom-right corner of the tile
- Rest tiles display an en-dash (–)
- The currently playing tile is highlighted: full-brightness neon color with a white LED flash burst on beat, decaying over ~150ms
- Inactive filled tiles display their pitch-class color dimmed (dark background, colored text)
- Empty slots (unreachable; all 8 are always filled with rests or notes) show a faint outline only

### UI — Note tile editing

- **2-axis drag** on any note tile (outside of recording mode):
  - Drag left/right → scrubs through the 12 pitch classes (every 20px = 1 semitone of pitch class)
  - Drag up/down → changes the octave (every 40px = 1 octave), clamped to MIDI range
  - The tile updates live during the drag; the change commits on mouse-up
- **Cmd+click** on a note tile toggles the step between a rest and its last non-rest pitch (the previous pitch is remembered and restored)

### UI — Control tiles

Control tiles support both click and vertical drag:
- **Click** (drag < 5px): cycles to the next option
- **Vertical drag**: scrubs through options (32px per step, wraps)

Controls:
- **Rate tile**: cycles/scrubs through all 12 rate options; displays current choice (e.g. "1/8")
- **Mode tile**: cycles/scrubs Forward → Reverse → Random; always shown active (pink)
- **Record tile**: click toggles recording; glows pink while recording is active
- **Rest tile**: click enters a silent step (only effective during recording)
- **Key tile**: cycles/scrubs through 12 root notes; highlighted when a scale is active
- **Scale tile**: cycles/scrubs through Off + 7 scales; highlighted when active

### UI — Design language

Dark hardware-inspired panel evoking a Torso T-1 or Polyend Tracker — matte black chassis, dense grid of backlit pads, neon LEDs at full saturation.

**Color spec (shipped):**
- **Panel background:** near-black (`#181820`) with a subtle top-to-bottom gradient overlay
- **Flat** — no bevels, no shadows
- **Tile colors:** 12 vibrant pitch-class neon colors — hot pink (C), rose-magenta (C#), violet (D), indigo (D#), blue (E), sky (F), teal (F#), green (G), lime (G#), yellow (A), orange (A#), red (B)
- **Inactive filled tile:** dark background (`#242432`), colored text at full saturation
- **Active (playing) tile:** colored background; white LED flash burst on beat, decays to pitch color over 150ms
- **Rest tiles:** dark background (`#242432`), dimmed color (35% alpha text)
- **Empty slots:** very faint outline only
- **Control tiles:** dark (`#242432`) when inactive; hot pink (`#e8347a`) when active; white text

---

## P1 Requirements

### Overdub / per-step punch-in

Deferred. Re-record replaces the entire sequence for now.

### Additional playback modes

- **Pendulum** — forward then reverse, no repeated endpoints
- **Drunken** — random with some continuity/weighting toward adjacent steps

---

## P2 Requirements

### Variable step count

- Support step counts below 8 — pentatonic (5), hexatonic (6), etc.
- Either a manual count selector or automatic from the chosen scale length

### Scale population

- Button to populate all 8 steps with the notes of the chosen scale in ascending order

---

## Out of scope

These are not planned for any current phase:

- Velocity per step
- Gate length per step
- Per-step probability
- Polyphony / chords per step
- Pattern chaining / song mode
- MIDI CC output
- Standalone app format
