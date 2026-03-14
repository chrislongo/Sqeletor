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

### P0 — MVP

Nothing ships without this. The tile UI ships at P0 — it's the product identity, not a later polish pass.

### P1 — First real feel

Note colors and scale/key awareness. The plugin becomes genuinely musical.

### P2 — Quality of life

Additional playback modes and flexible step counts for non-diatonic patterns.

---

## P0 Requirements

### Recording

- Up to 8 steps; 7 is the natural fit for diatonic patterns but 8 is the cap
- User hits **Record**, then plays notes on a MIDI keyboard; each note-on fills the next step in order
- **Rests** are entered via a dedicated Rest button (tapped instead of playing a note)
- Recording stops automatically when step 8 is filled, or manually when the user hits Record again
- Loop length = number of steps actually recorded (e.g. 4 notes → 4-step loop); no padding to 8
- Duplicate pitches supported — each tap = one step regardless of pitch
- Overdub (punch-in per step) is deferred; re-record replaces the entire sequence

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

### UI — Layout

- Single panel, fixed size (TBD — roughly 600×200px)
- **Top:** Rate selector (left)
- **Middle:** Up to 8 note tiles in a horizontal row; empty slots are visually distinct from filled ones
- **Bottom:** Playback mode buttons (left), Record button (center), Rest button (center-right)

### UI — Note tiles

- Each filled tile displays the note name in large text (e.g. "C", "F#")
- Rest tiles display a dash or rest symbol
- The currently playing tile is highlighted
- Empty (unrecorded) slots are visually subdued
- Tiles are a neutral color at P0; per-pitch-class colors are P1

### UI — Controls

- **Rate:** dropdown or segmented button, top-left
- **Playback mode:** three icon buttons (→, ←, shuffle), bottom-left; active mode highlighted
- **Record:** prominent button; glows/pulses while recording is active
- **Rest:** button active only during recording; enters a silent step

### UI — Design language

Sqeletor intentionally departs from the silver-grey Corvid Audio house style. The target vibe is Valhalla DSP — dark, electric, and fun.

- **Panel background:** near-black (e.g. `#0e0e1a` deep navy-black, or pure `#111111` — TBD)
- **Flat** — no bevels, no shadows (same rule as other Corvid plugins)
- **Text:** bold white; labels uppercase
- **Tile colors:** 12 vibrant, high-saturation pitch-class colors — purples, cyans, electric blues, magentas, hot pinks, lime greens. Full palette TBD at implementation; aim for Valhalla-style neon saturation, not pastels
- **Active tile:** brighter / higher-luminance variant of its pitch color
- **Rest tiles:** dim neutral (dark grey)
- **Empty slots:** very dark, barely-visible outline only

---

## P1 Requirements

### Note colors

- One vibrant, high-saturation color per pitch class (12 colors total)
- Rest tiles are grey/neutral
- Tile colors contrast against the silver-grey `#d8d8d8` panel background

### Scale / key

- Dropdown to select root note and scale/mode
- Populates the step slots with the notes of the chosen scale in ascending order
- Recorded notes snap to the selected scale (or this is how initial slot values are set — TBD)

---

## P2 Requirements

### Additional playback modes

- **Pendulum** — forward then reverse, no repeated endpoints
- **Skip** — TBD
- **Drunken** — random with some continuity/weighting toward adjacent steps

### Variable step count

- Support step counts below 8 — pentatonic (5), hexatonic (6), etc.
- Either a manual count selector or automatic from the chosen scale length

---

## Out of scope

These are not planned for any current phase:

- Overdub / per-step punch-in
- Velocity per step
- Gate length per step
- Per-step probability
- Polyphony / chords per step
- Pattern chaining / song mode
- MIDI CC output
- Standalone app format
