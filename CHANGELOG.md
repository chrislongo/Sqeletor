# Changelog

## v0.3 (2026-03-15)

### Added
- **Scale locking** — right-side column with Key (C–B) and Scale selectors.
  Scales: Off, Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian.
  Snap is non-destructive; stored notes are unchanged and display/playback
  reflect the snapped pitch in real time. Changing key or scale mid-play
  takes effect at the next note boundary.
- **All-rests init** — plugin launches with a full 8-step rest sequence so
  patterns can be built immediately by dragging, without recording first.

### Changed
- **2-axis tile drag** — left/right drag cycles pitch class (C→C#→…→B, wraps);
  up/down drag changes octave (clamped, no wrap). Replaces the previous
  shift+drag scheme.
- **Rest removed from drag cycle** — dragging no longer passes through rest.
  Use Cmd+click to toggle a step to/from rest.

## v0.2 (2026-03-14)

### Added
- Dark neon theme with per-pitch-class tile colors (Vital-style palette).
- LED flash: active tile bursts white on each step, decays over 150 ms.
- Swap drag: drag one note tile onto another to reorder steps.
- Octave label rendered in the bottom-right corner of each tile.
- Mode parameter sync fix: mode changes now take effect cleanly mid-sequence.

### Changed
- UI layout updated to 2×4 grid (8 steps across two rows).

## v0.1 (initial)

- Initial plugin scaffolding: MIDI passthrough, APVTS parameter layout,
  basic tile UI, rate and mode selectors, record and rest buttons.
