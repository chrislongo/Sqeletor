# Sqeletor — Logic MIDI FX Plugin Idea

A Logic MIDI FX Sequencer

The name: A fun play on Skeletor and Sequencer.

## Core tenets

- Simplicity first — do a few things really well, not a million things
- Start simple, gradually build functionality
- UI is fun, large, flat, and colorful
- Fast. Feels like a real instrument. Does not require a lot of thought to use
- Rewards experimentation and happy accidents
- Free and open source

## UI concept

- Horizontal row of big colored tiles — one per note slot
- Each tile shows the note name in large text
- The active (currently playing) tile is highlighted/flashes
- Controls: rate selector top-left, playback mode buttons bottom-left (forward/reverse/random), play button bottom-right
- The tile UI *is* the product — it's what makes this different from every other sequencer

---

## P0 — MVP (nothing works without this)

1. **Host sync** — Read host tempo and transport; fire MIDI notes on the grid
2. **Note slots** — User enters/selects notes per slot. Start with 7 (maps cleanly to diatonic scales); slot count may flex to 5–8 later
3. **Rate selection** — 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 against host tempo
4. **Playback modes** — Forward, reverse, random (shuffle/permutation — not pure random, which feels broken)
5. **Big block tile UI** — Large colored tiles with readable note names; the visual identity ships in P0, not later

## P1 — First real feel

6. **Note colors** — Vibrant per-pitch-class color scheme
7. **Scale/mode dropdowns** — Populate slots from a chosen scale and root

## P2 — Quality of life

8. **More playback modes** — Pendulum, skip, drunken, etc.
9. **Variable slot count** — Support pentatonic (5) and other scale lengths

---

## Research

See [RESEARCH.md](RESEARCH.md) for competitive landscape.
