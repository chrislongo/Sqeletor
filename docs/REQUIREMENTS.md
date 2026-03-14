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

## Plugin format

- **Type:** AUv2 MIDI Effect — Logic MIDI FX slot
- **Host:** Logic Pro (primary); any AU MIDI FX host (secondary)
- **Autonomous:** Plays back without held MIDI input
- **Host sync:** Tempo and transport from host (start/stop/position)

### Supported track types in Logic

- **Software instrument tracks** — sequences any Logic or third-party AU instrument
- **External MIDI tracks** — sequences hardware synths, drum machines, Eurorack MIDI-CV, etc. via any connected MIDI port/channel; no IAC bus or extra routing needed

---

## Functional requirements

### Recording

- Max 8 steps
- User hits **Record**, then plays notes on a MIDI keyboard; each note-on fills the next step in order
- **Rests** are entered via a dedicated Rest button (tapped instead of playing a note)
- Recording stops automatically when step 8 is filled, or manually when the user hits Record again
- Loop length = number of steps actually recorded (e.g. 4 notes → 4-step loop); no padding to 8
- Duplicate pitches supported — each tap = one step regardless of pitch
- Overdub (punch-in per step) is out of scope for now; re-record replaces the entire sequence

### Playback

- Plays back the recorded sequence in a loop, locked to host tempo and transport
- Follows host play/stop; resets to step 1 on stop or rewind
- No playback when host is stopped

### Rate

- Playback rate relative to host tempo
- Options: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32

### Playback modes

- **Forward** — steps 1→N, loop
- **Reverse** — steps N→1, loop
- **Random** — shuffle/permutation (each step plays once per cycle before repeating; not pure random)

### MIDI output

- Emits Note On / Note Off per step
- Rest steps emit nothing
- Velocity: fixed (default 100) for now
- Gate: fixed (~80% of step duration) for now

---

## UI requirements

### Layout

- Single panel, fixed size (TBD — roughly 600×200px)
- **Top:** Rate selector (left)
- **Middle:** Up to 8 note tiles in a horizontal row; empty slots are visually distinct from filled ones
- **Bottom:** Playback mode buttons (left), Record button (center), Rest button (center-right)

### Note tiles

- Each filled tile displays the note name in large text (e.g. "C", "F#")
- Rest tiles display a dash or rest symbol
- Tiles are colored — one vibrant color per pitch class (12 colors); rests are grey/neutral
- The currently playing tile is highlighted
- Empty (unrecorded) slots are visually subdued

### Controls

- **Rate:** dropdown or segmented button, top-left
- **Playback mode:** three icon buttons (→, ←, shuffle), bottom-left; active mode highlighted
- **Record:** prominent button; glows/pulses while recording is active
- **Rest:** button active only during recording; enters a silent step

### Design language

- Consistent with other Corvid Audio plugins: flat, no bevels or shadows
- Panel background: silver-grey `#d8d8d8`
- Tile colors: vibrant, high-saturation (contrast with the grey panel)
- Note name text: bold, white or dark depending on tile color contrast

---

## Out of scope (for now)

- Overdub / per-step punch-in
- Velocity per step
- Gate length per step
- Per-step probability
- Polyphony / chords per step
- Pattern chaining / song mode
- Scale/mode dropdown
- Pendulum, skip, drunken playback modes
- MIDI CC output
- Standalone app format
