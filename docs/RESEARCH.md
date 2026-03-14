# Sqeletor — Market Research

## The space

Logic Pro's MIDI FX slot (AUv2 MIDI Effect) is underserved. The AU format requirement eliminates most VST-only tools, which thins the competition considerably. What remains splits into three camps:

1. **Complex power sequencers** (Stepic, Thesys, SEQUND) — feature-rich but dense
2. **Generative/probabilistic sequencers** (Riffer, Stochas, Harmony Bloom) — sculpt patterns, don't draw melodies
3. **Chord/harmony tools** (Scaler, Cthulhu, Phrasebox) — require held input notes; not autonomous

No clean, simple, autonomous melodic step sequencer with a great UI exists for Logic MIDI FX.

---

## Key competitors

### Stepic — Devicemeister (€39)
AU MIDI FX. The most complete melodic step sequencer in the Logic space. Won 2024 KVR Readers' Choice. 16-step polyphonic sequencer, 9 playback modes (forward/reverse/pendulum/drift/random/etc.), built-in scale system, 8 mod lanes, 200+ randomization options.

**Gap:** Dense and "not particularly fun." UI is heady — too much to learn before you can make music. No manual.

### Thesys — Sugar Bytes (~€99)
AU MIDI FX. Long-established (2012). 32 steps, 5 parallel lanes (pitch/velocity/gate + 2 CC), scale-aware pitch lane, MIDI drag-and-drop export.

**Gap:** Dated UI, expensive, limited playback modes. The incumbent nobody loves.

### Riffer 3 — Audiomodern ($54, often ~$29)
AU. Generative riff creation with 53 built-in scales. Per-note locking while randomizing others. Great for inspiration.

**Gap:** Generative paradigm, not step-programmed. You sculpt, you don't draw. Minimal playback direction modes.

### Stochas — Surge Synth Team (Free)
AU MIDI FX. Probability-first step sequencer: per-note trigger chance, timing jitter, velocity randomization. 4 layers, 64 steps, key/scale lock.

**Gap:** Drum-sequencer DNA; melodic use is secondary. No playback direction modes. Utilitarian UI.

### Scaler 3 — Plugin Boutique (~$49)
AU MIDI FX. Best-in-class harmony/chord tool with scale detection, voice leading, progression suggestions. Scaler 3 adds melodic/bass lanes.

**Gap:** Chord tool first, sequencer second. Known MIDI latency bug in Logic 11/12 with Apple instruments. Not a step sequencer.

### Cthulhu — Xfer Records ($39)
AU (routing-dependent in Logic). Chord bank + arpeggiator. Requires held input notes.

**Gap:** Not a step sequencer. Development stalled. Logic routing is painful.

### Phrasebox 2 — Venomode (~$49)
AU MIDI FX. Piano-roll-based phrase arpeggiator. Draws phrases against held chord input. Per-note probability/CC envelopes.

**Gap:** Requires held input. More "mini DAW" than step sequencer.

### Harmony Bloom — Mario Nieto World (~$39)
AU MIDI FX. Visual/euclidean sequencer with probability and scale awareness. Generative and ambient-oriented.

**Gap:** Generative paradigm; melody is not explicit. Good for texture, not precise melodic programming.

### Fugue Machine Rubato — Alexandernaut (~$10 iOS / macOS AUv3)
AUv3. Multi-playhead piano roll — one pattern played simultaneously by up to 4 heads with independent speed/direction/pitch offset.

**Gap:** No scale awareness at all. Piano roll paradigm, not step grid. iOS-first.

### Logic Pro native Step Sequencer (free, built-in)
Introduced in Logic 10.6. Grid-based "pattern regions" in the arrangement — each region is a step sequencer that plays on its track. Rows can be pitched (melodic) or unpitched (drum). Per-step controls: velocity, gate length, skip, tie, note repeat. Logic 12 (2025) added new playback modes: Pendulum, Brownian, Arp1, Arp2, plus scale lock (snap all steps to a chosen scale/root). Up to 32 steps per row, polyrhythmic (independent step count per row). Tight host integration: tempo-synced, follows transport, no latency issues.

**Gap:** Fundamentally a region/clip editor, not a MIDI FX plugin. You can't insert it in the MIDI FX slot on an instrument track — it only works as pre-recorded pattern regions in the arrangement. This means it can't be used as a live performance device, can't be automated like a plugin, and can't be stacked or chained with other MIDI FX. No per-step probability. No third-party extensibility. And because it's region-based, you have to commit to a pattern before you hear it on a synth — there's no "spin it up on a track and noodle" workflow. **This is exactly the gap a Logic MIDI FX step sequencer fills.**

---

## Market gaps

**1. Simple, immediately playable melodic step sequencer as Logic MIDI FX**
The "obvious" plugin doesn't exist. The market bifurcates between complex power tools and generative tools. Nothing hits: "here are 8 steps, each plays a note, hit play, it loops" in a Logic MIDI FX insert with a good UI.

**2. Scale-degree-first note entry**
Every plugin thinks in absolute pitch (MIDI note numbers, piano keys). None let you program in scale degrees — so transposing or changing key means re-entering every note. A plugin where slot 1 = root, slot 3 = third, etc. would be distinctively more musical.

**3. Rich playback modes + scale awareness + simple UI in one package**
Stepic has playback modes and scale support but is complex. No plugin cleanly combines: multiple playback direction modes + scale-aware note entry + an approachable, fast UI.

**4. Readable note display**
Most plugins show tiny numbers or semitone offsets on a miniature piano. No plugin puts large, readable note names per step front-and-center. This is a genuine visual design opportunity.

**5. Autonomous playback (no held input)**
Many Logic MIDI FX plugins (Cthulhu, Phrasebox, BlueARP) require held notes to do anything. Self-contained sequencers that run without input are rarer and more useful for Logic instrument tracks.

---

## UI/UX patterns in the field

| Paradigm | Examples | Strength | Weakness |
|---|---|---|---|
| Horizontal step grid + pitch row | Thesys, Logic native | Familiar, temporal clarity | Pitch is a number; no harmonic context |
| Probability grid | Stochas, Riffer | Shows variation | Hard to read as a melody |
| Mini piano roll | Phrasebox, Fugue Machine | Flexible note lengths | Loses step feel; complex to edit |
| Abstract visual/euclidean | Harmony Bloom | Engaging | Melody not explicit |
| Multi-lane (pitch/vel/gate) | Stepic, SEQUND, Thesys | Parameter separation | Dense; high cognitive load |

**The gap:** No plugin labels note names or scale degrees in a large, readable font per step. Everyone uses small numbers or a mini keyboard.

---

## Pricing

| Tier | Range | Examples |
|---|---|---|
| Free | $0 | Stochas, BlueARP |
| Budget | $29–39 | Cthulhu, Stepic, SEQUND (sale), Riffer (sale) |
| Mid | $40–59 | Riffer (full), Scaler 3, Phrasebox 2, MelodicFlow |
| Premium | $80–130 | Thesys |

**Sweet spot for a new entrant:** $29–49. Above $59 is hard to justify for a focused tool. Riffer's heavy discounting ($54 → $19) signals market pressure to go lower.

---

## Hardware sequencers

Not direct competitors, but the workflow paradigm Sqeletor draws from.

### Roland TR-808 / TR-909 — the step grid paradigm
The 808 (1980) defined the mental model still used everywhere: rows are instruments, columns are time steps, lit buttons are hits. Purely binary — on or off per step. The 909 (1983) added **programmable shuffle** (swing as a percentage that delays offbeats) and per-track accent, making groove a programmable parameter rather than a fixed characteristic. These two machines established that a sequencer's UX should let you see and toggle the entire pattern at a glance while it plays.

### Roland TB-303 — accent, slide, and the accidental interface
The 303 (1981) programs pitch and rhythm in two separate phases — a famously confusing workflow that led to acid house through happy accidents. Its enduring contributions: **slide** (portamento that also suppresses envelope retrigger, creating a fluid "breathing" quality between notes) and **accent** (boosts both volume and filter envelope depth on marked steps). Both are per-step flags, not continuous gestures. Every bassline sequencer since has copied this model.

### Elektron — p-locks and conditional trigs
Elektron's sequencer (Machinedrum 2001, through Digitakt, Syntakt, etc.) looks like a 16-step grid but each step is a container for an entire parameter snapshot. **Parameter locks (p-locks):** hold a step + turn any knob = that step gets its own unique value for any parameter. A single looped pattern can contain what would normally require many bars of DAW automation. **Conditional trigs:** each step carries a logic condition — play every 2nd loop, play at X% probability, play only if the previous trig fired, etc. A looped pattern never repeats identically. P-locks are arguably the most influential sequencer concept of the past 25 years, copied by almost every modern hardware sequencer.

### OXI ONE (~$499, MKII 2025)
4–8 independent sequencer engines (configurable as up to 64 tracks), each running a different mode simultaneously: standard step, polyphonic, chord, drum, **stochastic** (each column is a pitch, pad height = probability that pitch fires), or matriceal (generative/Markov). Per-step micro-timing. Strong scale quantize. Battery powered, Bluetooth MIDI. The stochastic mode is genuinely distinctive — you program a probability landscape rather than a fixed sequence.

### Squarp Hapax (~$850)
16-track polyphonic sequencer, 192 PPQN (highest in class), full MPE recording. The key architectural idea: **each track is a signal chain** — notes pass through a stack of non-destructive MIDI effects (arpeggiator, euclidean rhythms, probability, harmonizer, scaler, swing, LFO, etc.) before output. Changing the scale or adding randomness is non-destructive; the source notes are unchanged. Dual-project playback (two full projects run simultaneously) enables seamless live set transitions. 64 automation lanes per track. The MIDI-FX-chain model is beginning to influence firmware updates across other sequencers.

### Sequentix Cirklon (~€1,750, Cirklon2 2025)
Boutique, cult-status sequencer with historically years-long waitlists. Up to 64 tracks. P3 pattern format shows pitch, velocity, length, delay, and 4 auxiliary values per step simultaneously — all editable without mode switching. **Per-MIDI-port clock offset** (0.25ms granularity) compensates for each synth's response latency so all instruments hit simultaneously — a feature no other sequencer offers. Inter-pattern data fetching allows one pattern to reference and transform another's values. Reference-quality MIDI timing. No graphical UI — entirely text and numbers. Rewards obsessive mastery.

### Arturia BeatStep Pro (~$249)
Two 64-step monophonic melodic sequencers + a 16-track drum sequencer. The defining feature: **one rotary knob per step** sets pitch — you can see the melodic contour as a physical shape across the row. Scale lock constrains knobs to a chosen scale. Forward/reverse/random/ping-pong playback modes. CV/Gate + MIDI DIN + USB. No chords. No MIDI CC sequencing.

### Arturia KeyStep 37 Mk2 (~$229, launched Feb 2026)
37-key keyboard + single-track 64-step polyphonic sequencer. **Keyboard is the primary entry method** — play notes to record steps in real-time or step-by-step. Scale lock + Chord mode: memorize a chord's intervals, replay it transposed with one finger. Mk2 adds **Mutate** (randomizes pitch/rhythm/density live) and **Spice** (randomizes gate/ratchet). CV + MIDI DIN + USB-C.

### Arturia KeyStep Pro (~$499)
4 independent polyphonic sequencer tracks (up to 64 steps, up to 16 notes per step) + drum track. Same keyboard-entry paradigm. Each track has its own step count and time division → polyrhythm. Per-step micro-timing (nudge steps ahead/behind the grid). Scale lock + Chord mode. 4 sets of CV/Gate out + dual MIDI DIN out. No MIDI CC sequencing.

### Key concepts across the hardware landscape

| Concept | Origin | Relevance to Sqeletor |
|---|---|---|
| Step grid (rows = instruments, cols = time) | TR-808 | Foundation of the tile row paradigm |
| Programmable swing/shuffle | TR-909 | Future "feel" parameter |
| Slide / accent as per-step flags | TB-303 | Gate length + velocity variation per step (P2) |
| Keyboard-as-input | Arturia KeyStep | Core P0 recording paradigm |
| P-locks (per-step parameter snapshots) | Elektron | Informs future per-step velocity/gate |
| Conditional trigs / probability | Elektron, OXI | Future "drunken" / chance mode |
| MIDI FX chain (non-destructive) | Hapax | Scale snap, future effects |
| Stochastic / probability landscape | OXI ONE | Future generative playback mode |

---

## The opening for Sqeletor

The gap is real and specific:

- **Logic Pro AU MIDI FX format** — eliminates most competition
- **Autonomous** — no held input needed, works as a self-contained melody engine
- **Fast to use** — open it, pick notes, hear music; no manual required
- **Scale-aware** — at minimum, snap output to a scale; ideally, program in scale degrees
- **Multiple playback modes** — forward, reverse, random at minimum; pendulum etc. later
- **Large, readable UI** — note names front-and-center in big tiles; the visual identity is also the UX

Stepic is the closest analog (scale support + rich playback modes + Logic AU MIDI FX) but it's complex and dense. Thesys is the historical default but aging and expensive. Neither is fun or fast.
