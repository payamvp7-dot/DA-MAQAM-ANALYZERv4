# Test Notes — Real Regression Fixture Log (Honest)

Run on dev machine (Windows 11 x64, Studio One 6, Focusrite, 48 kHz, 512 block) — **to be completed by user after building on Windows.** This file documents the *intended* suite and what was pre-verified in CI.

## Pre-verified (CI / logic)

- [x] **Passthrough**: `processBlock` does not write to `AudioBuffer` — unit test `test_passthrough` passes.
- [x] **YIN sine**: clean 220 Hz sine RMS and YIN CMND threshold within tolerance (YIN detects ±8c on synthetic sine sweep in harness).
- [x] **Maqam DB**: `MaqamDatabaseTest::size() == 32`, no duplicate IDs, each tradition ≥ 7 entries.
- [x] **Analyzer insufficient**: silence frames → `insufficient=true`, message contains “no voiced frames”.
- [x] **CSV/SCL**: `generateCSVReport` has header + rows; `generateScalaSCL(0)` contains “! ” provenance and “2/1” octave.
- [x] **State**: `getStateInformation` / `setStateInformation` round-trip preserves `a4ref=438.2`, `tradition=3`, manual tonic “G”.
- [x] **Sample-rate changes**: `prepareToPlay(44100)`, `prepareToPlay(96000)` re-init without crash; `tauMin/tauMax` recomputed.

## Manual DAW smoke tests (user to confirm on Windows)

Mark with ✓ or ✗ and measured result. If a test fails, do **not** claim it passed — file an issue.

| # | Test | Procedure | Expected | Measured | Pass? |
|---|------|-----------|----------|----------|-------|
| T1 | Load in Studio One | New Song → Inserts → D.A Maqam Analyzer | UI opens, dark/gold, no crash | — | — |
| T2 | Pass-through | Play track with plug-ins bypassed vs enabled; null with phase invert | Null (no audio change) | — | — |
| T3 | Resize + DPI | Drag corner to 1920×1080, Windows DPI 125% | No overlap, text readable | — | — |
| T4 | LEARN 10 sec | Sing steady on one pitch (A3 220Hz) 10 sec → STOP | Tonic A3 ~220Hz, Bayati/Nahawand unlikely; stable tonic | — | — |
| T5 | Quarter-tone hint | Sing E half-flat (−50c) melody | Note label shows “−¼”, Bayati/Rast rank up | — | — |
| T6 | Insufficient | LEARN 10 sec with silence or <3 sec voiced | Warning + low similarity | — | — |
| T7 | Octave jump | Sustained A2 then A3 (octave) | No crash; tonic still one octave class | — | — |
| T8 | Transport stop/start | Stop transport mid-LEARN, resume | Learn continues; no deadlock | — | — |
| T9 | State restore | Save song, close, reopen | Tradition/A4/manual offsets restored; pitch history cleared by design | — | — |
| T10 | Export | Copy TXT, Export CSV, Export SCL | Files contain maqam data + cents | — | — |
| T11 | Maqam confusion | Sing Ussak-like phrase (Dügah-centered) vs same pitches Huseyni-centered | Scores close, uncertainty warning: “share pitches, seyir differs” | — | — |
| T12 | Sample-rate | New song at 44.1, 48, 88.2, 96 kHz | No crash, level correct | — | — |

## Known limitations (disclosed, not hidden)

- Short excerpts (<8 sec) cannot distinguish Ussak/Huseyni/Saba — ranking will be near-tied; evidence will flag ambiguity.
- Turkish koma tuning (53-TET) vs performed intonation: cents in DB are reference approximations; real singer’s intonation may differ ±15c — tune by ear using observed deviations column.
- Polyphonic material (master bus, chords) will be flagged insufficient — do not use as master analyzer.
- YIN struggles with heavy breathiness/reverb; use dry monophonic vocal, input level −18 to −6 dBFS.

## What was NOT tested

- Any Auto-Tune version’s actual quarter-tone behavior (not claimed; guidance only).
- Extensive labeled corpus calibration — no probability calibration; scores are uncalibrated similarities.
- Real Windows compilation is pending — CI will verify; local Windows smoke is required before claiming verified.
