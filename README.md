# Sqeletor

A Logic Pro MIDI FX step sequencer. Open it, record a phrase, hear it loop in under 30 seconds.

Sqeletor lives in Logic's **MIDI FX slot** — drop it on any software instrument or external MIDI track, hit Record, play up to 8 notes on your keyboard, and the loop plays back immediately in sync with Logic's transport. Works with hardware synths the same way, no IAC bus or extra routing needed.

Free and open source.

---

## How it works

1. Drop Sqeletor into a Logic MIDI FX slot
2. Set the playback rate and mode
3. Hit **Record** and play up to 8 notes on your MIDI keyboard (or tap **Rest** for silent steps)
4. Recording stops automatically when all steps are filled, or tap Record again to stop early
5. Hit Play in Logic — Sqeletor loops your phrase in sync with the host

To re-record, hit Record again. The new phrase replaces the old one.

## Features

- Up to 8 steps; loop length equals the number of notes recorded
- Playback rates: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
- Playback modes: Forward, Reverse, Random (shuffle — each step plays once per cycle)
- Host transport sync — follows Logic's play/stop, resets on rewind
- Large tile UI with note names front-and-center, one vibrant color per pitch class

## Requirements

- macOS
- Logic Pro (or any AUv2 MIDI Effect host)

## License

GPL v3 — see [LICENSE](LICENSE).
