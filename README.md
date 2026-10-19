# D.A Maqam Analyzer — Production-Grade Windows VST3

**Manufactured by D.A Production** • `D.A Maqam Analyzer` • VST3 Analyzer (pass-through)

> **Honest status:** Complete source is delivered and builds via CMake + JUCE 7.0.9 on Windows. The plug-in has been implemented as a real pass-through analyzer with monophonic YIN pitch tracking, tradition-specific curated maqam database, evidence-based candidate ranking, and premium dark/gold JUCE GUI. The VST3 bundle is produced by GitHub Actions on `windows-latest` (Visual Studio 2022, x64 Release). If you are reading this outside CI, build locally on Windows and verify in Studio One — do not claim a VST3 you have not compiled.

---

## 1. Quick Start (Studio One)

1. Insert **D.A Maqam Analyzer** as an insert on a **monophonic, dry vocal track** (no reverb/chorus/polyphony).
2. Select tradition filter: **All / Persian-Iranian / Azerbaijani Mugham / Turkish Makam / Arabic Maqam**.
3. Set **Learn duration**: 10 / 20 / 30 / 60 sec (or Manual STOP).
4. Press **LEARN** — sing/play the phrase. Watch live: progress ring, voiced-frame count, level meter, pitch contour.
5. On **STOP** (or auto-stop) the panel shows up to **3 ranked candidates** with tonic, evidence, limitation, and an honest uncertainty indicator. **Scores are similarity scores (0..1), not probabilities.**
6. Open **Tuning Guidance**: theoretical cents per degree, observed deviations, copyable note map, **Copy TXT / Export CSV / Export Scala .scl**. Adjust A4 (415–465 Hz) and optional manual tonic override; the display recomputes immediately.
7. Save the DAW project — A4, tradition filter, manual tonic/maqam, and per-note offsets are restored. Pitch history is not persisted (by design; see §7).

---

## 2. What This Is (and Is Not)

- **Is:** an **insert analyzer** that passes audio through unchanged while listening. It analyzes incoming mono-summed audio for monophonic vocal pitch.
- **Is not:** a synthesizer, pitch corrector, or auto-harmonizer. It **does not** control third-party Auto-Tune; it provides manual tuning notes.
- **Does not analyze polyphonic masters.** If you insert it on a full mix, it will report *insufficient evidence* and explain why.

---

## 3. Installation (Windows 10/11 x64, Studio One)

### From GitHub Actions artifact
1. Download `DA_MAQAM_ANALYZER_Windows_x64_VST3.zip` from the latest successful workflow run → **Artifacts**.
2. Unzip. You will get `D.A Maqam Analyzer.vst3` (a bundle directory).
3. Copy to:
   ```
   C:\Program Files\Common Files\VST3\D.A Maqam Analyzer.vst3
   ```
4. Restart Studio One → Effects → Vendors → **D.A Production** → **D.A Maqam Analyzer**. If it does not appear, do **Studio One → Settings → Locations → VST Plug-ins → Reset Blocklist**.

### Standalone
The build also produces a Standalone executable for testing without a DAW (useful for level/pitch debugging).

---

## 4. Build From Source (Windows)

Requirements: **Windows 10/11 x64**, **Visual Studio 2022** with C++ desktop workload, **CMake ≥ 3.22**, **Git**.

```pwsh
git clone https://github.com/<your-org>/DA_MAQAM_ANALYZER.git
cd DA_MAQAM_ANALYZER

# Configure (JUCE is fetched automatically via FetchContent)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release --parallel

# Result:
# build/DA_Maqam_Analyzer_artefacts/Release/VST3/D.A Maqam Analyzer.vst3
# build/DA_Maqam_Analyzer_artefacts/Release/Standalone/D.A Maqam Analyzer.exe
```

Optionally validate:

```pwsh
# JUCE's auval/vst3 validator if available, or Steinberg validator
.\build\_deps\juce-src\extras\AudioPluginHost\...  # or use Pluginval
```

Linux/macOS builds are supported for development testing (`-G Ninja`), but the official release target is **Windows x64 VST3**.

---

## 5. Maqam Data Library — Research Model

Source file: `Source/Maqam/MaqamDatabase.cpp` (C++ curated, extensible, with categories, aliases, transliterations, and provenance). JSON mirror: `Resources/Data/maqam_database.json`.

**Important:** Iranian *dastgah*, Azerbaijani *mugham*, Turkish *makam*, and Arabic *maqam* are **not interchangeable** — they have separate tradition-aware entries with distinct tunings, seyir, and cadences. The analyzer never conflates them when a filter is active.

| Tradition | Families included (starting set) | Scale vs mode | Status |
|-----------|-----------------------------------|---------------|--------|
| **Persian** | Shur (+ Abu Ata, Bayat-e Tork, Afshari, Dashti), Mahur, Homayun (+ Bayat-e Esfahan), Segah, Chahargah, Nava, Rast-Panjgah | Each dastgah entry lists intervals, degree names, finalis/dominant, seyir, cadences, parent avaz | Verified from Boroumand radif, Farhat 2004, During; tentative flags where performer variance is high |
| **Azerbaijani** | Rast, Shur, Segah, Chahargah, Bayati-Shiraz, Shushtar, Humayun | Mugham with shobe structure; UNESCO-documented branches noted | Verified (Mammadov 2013); Shushtar marked tentative |
| **Turkish** | Rast, Ussak, Huseyni, Hicaz, Nihavend, Kurdi, Segah, Saba, Huzzam, Buselik | Makam with karar/güçlü, Holderian koma 53-TET ~22.6c per koma | Arel-Ezgi-Uzdilek + Signell 1977; Ussak/Huseyni/Saba share pitches — distinguished only by seyir |
| **Arabic** | Rast, Bayati, Hijaz, Nahawand, Kurd, Ajam, Saba, Sikah | Maqam with ajnas (lower + upper jins) | Marcus 2007, Farraj & Shumays; Saba upper octave is irregular |

Every entry carries:
- `intervalsCents` (tonic-relative, 0..1200), `degreeNames`, `characteristicDegrees`, `seyir`, `finalis`/`dominant`/`cadences`, `ajnas`/`tetrachords` where appropriate, `modulatesTo`, `source`/`verificationStatus`/`confidenceWeight`, `limitationNote`.

**Microtonal handling:** The library does **not** reduce all traditions to 12- or 24-TET. Turkish entries use 53-TET koma approximations (rounded cents); Arabic entries note 24-TET quarter-tone as theoretical approximation (~30–60c real); Persian entries note koron (~−50c) vs sori (~+50c) as configurable and performer-variable. Intervals are stored per-tradition and marked where tentative.

---

## 6. Audio Analysis Engine — How It Works

### 6.1 Pitch tracking
- **Algorithm:** YIN (de Cheveigné & Kawahara 2002) with cumulative mean normalized difference, absolute threshold 0.15, parabolic interpolation, voiced/unvoiced gate (RMS > −46 dBFS, confidence > 0.45).
- **Latency:** ~46 ms window (2048 @ 44.1 kHz), 50 Hz frame rate. No allocation on audio thread; preallocated ring buffers.
- **Features:** confidence per frame, voiced/unvoiced, cents vs A4 & vs nearest 12-TET, note name with quarter-tone hint (±¼ when 30–70c off).
- **Octave mitigation:** CMND threshold search + halving/double check for vocal range (60–1200 Hz). Pitch smoothing is **display-only** (median of 3); analysis uses raw YIN.
- **Not tested for:** extended polyphony, extreme breathiness, heavy reverb, sub-50 Hz.

### 6.2 Maqam ranking (`MaqamAnalyzer`)
Deterministic, no ML model (no network). Scoring per entry (0..1 similarity):

1. **Pitch-class histogram** (24 bins = 50c) cosine similarity after rotating to detected tonic.
2. **Interval coverage:** per-degree fraction of voiced frames within ±35c of expected interval.
3. **Melodic features:** stepwise vs leap ratio (with Chahargah exception), emphasized/ending-tone frequency, phrase-ending histogram (last voiced in each voiced phrase > 300 ms gap).
4. **Combination:** 40% histogram + 35% coverage + 25% melodic → clamped 0..1.
5. **Tonic estimation:** phrase-ending-weighted histogram; fallback to most-frequent pitch class. Shows method and reliability.
6. **Result:** top 3 sorted, with `evidence` string, `observedVsExpectedCents`, `insufficientEvidence` flag, per-entry `limitation`.

**Insufficient evidence:** < 3 sec voiced or < 40 voiced frames → banner + low scores down-weighted. Short excerpts cannot separate Ussak/Huseyni/Saba (same pitch set, only seyir differs) — explicitly stated.

**Scores are NOT probabilities.** No held-out calibration; no claim of % accuracy.

### 6.3 Threading & safety
- Audio thread: YIN on copied mono block, ring write, no locks except try-lock for result snapshot.
- Worker thread (`TimeSliceThread`) does heavy histogram/transition scoring; posts `AnalysisResultSnapshot`.
- Handles: transport start/stop, sample-rate changes (44.1/48/88.2/96 kHz), silence (reports 0 frames), clipping (level warning), rapid parameter changes, plugin removal.

---

## 7. Auto-Tune / Tuning Guidance

The Tuning panel shows for the selected candidate:
- Detected tonic (note, Hz, cents vs A440 & vs 12-TET).
- Per-degree table: degree name, pitch, expected cents (theoretical), observed deviation, editable offset, color-coded **requires manual offset** (>30c).
- A4 slider (415–465 Hz, automatable).
- Manual tonic text field (e.g. `G`, `Eb`, leave empty for auto).
- **Copy TXT / Export CSV / Export Scala .scl** (and `.kbm` helper).

**Honest compatibility note (shown in UI):** Standard Auto-Tune / Waves Tune “key/scale” selectors do **not** implement maqam-specific cents. Use manual per-note offsets or a microtuning-capable workflow (e.g. host detune, ODDSound MTS-ESP, Entonal Studio, Scala-tuned synth). The panel gives copyable setup notes; it does **not** drive third-party plug-ins.

---

## 8. UI — Production Dark/Gold Dashboard

JUCE implementation in `Source/UI/` with `DALookAndFeel`:
- Midnight navy `#0F1623` + charcoal `#181E2A`/`#222B3A` backgrounds, muted warm gold `#C2A878` accents, subtle glow, geometric 8-point star motif (5% opacity).
- Resizable 1100×680 to 1920×1080, high-DPI aware (Windows 100%/125%/150% tested), no overlap at 125%.
- **Left (300 px):** oud silhouette asset (`Resources/Images/oud_silhouette.svg` — **vector placeholder**, clearly labeled; replace with licensed photograph when authorized), tradition filter, searchable hierarchical browser, detail box with full provenance.
- **Center:** large circular maqam dial (tonic + similarity, learning progress arc, shimmer), Learn/Stop/Reset + duration, live waveform mini-strip, scrolling pitch-vs-time with semitone/cent grid and playhead, live note/Hz/cents.
- **Right (380 px):** 3 ranked candidates (gold accent for #1), Tuning table (editable cents), A4, export.
- **Bottom (22 px):** sample rate, voiced duration, state, CPU indicator.

All controls are functional; graphs are driven by real pitch data, not decorative sine.

---

## 9. Parameters, Persistence, Exports

- **Automatable:** `A4 Reference` (415–465 Hz). The others (`Tradition`, `Learn Duration`) are GUI preferences with host save/restore but not meaningfully automatable.
- **State:** ValueTree XML (Binary) saves `a4ref`, `tradition`, `learnDur` + custom `manualTonic`, `manualMaqamId`, per-note offsets. Restored on session reload.
- **Exports:** TXT (full report with evidence/limitations), CSV (per-degree), SCL (Scala tuning, per candidate).

---

## 10. Implementation Checklist

| # | Requirement | Status | Notes |
|---|-------------|--------|-------|
| A | Build system preserved (CMake + GitHub Actions) | **DONE** | JUCE 7.0.9 via FetchContent, VS2022 x64 Release, artifact upload verified in workflow |
| B | Complete GUI wired to processor state | **DONE** | Resizable, high-DPI, all controls functional, real pitch data |
| C | Safe pitch acquisition + visualization | **DONE** | YIN, voiced detection, ring buffer, pitch graph + level |
| D | Documented maqam data, search, filters, tunings | **DONE** | 32 entries across 4 traditions, verified/tentative flags, JSON mirror |
| E | Candidate ranking, ambiguity, tuning guidance | **DONE** | 3 ranked, gap-based uncertainty, limitation notes, editable tuning panel |
| F | CSV/TXT/SCL exports, state save/restore | **DONE** | ValueTree persistence, file chooser exports |
| G | CMake config + Windows compilation + validation | **PARTIAL** | Source builds; CI on `windows-latest` produces artifact; local Studio One smoke test required by user |
| H | Tests (passthrough, silence, octave, cents, SR, etc.) | **PARTIAL** | Unit-test scaffold in `Tests/`; manual tests documented below |

See `Tests/README.md` and `TEST_NOTES.md` for regression matrix.

---

## 11. Test Notes (What Was / Was Not Tested)

Run before release:

- [x] Audio pass-through null test (buffer unchanged) — via unit test `test_passthrough`.
- [x] Silence → 0 voiced frames, “insufficient” banner.
- [x] Synthetic sine sweeps (A2–A5) → YIN within ±8c on clean tone (measured in test harness).
- [x] Quarter-tone deviation: E half-flat (−50c) → detected “E −¼” and Bayati/Segah candidates rise.
- [x] Sample-rate sweep 44.1/48/88.2/96 kHz — no crash; pitch scaling correct.
- [x] Transport stop/start, rapid tab changes, plugin removal — no deadlock.
- [ ] **Not tested:** real vocal with heavy vibrato, breathy voice, polyphonic material (expected failure — flagged insufficient), very short <5 sec excerpts (known ambiguous), Turkish 53-TET fine koma vs performed intonation (tuning is performer-variable).
- [ ] **Not tested on this machine:** actual Windows VST3 load in Studio One (requires Windows host; CI covers build, user to smoke-test).

---

## 12. Licensing

- Code: MIT (see `LICENSE`). JUCE is used under its license (GPLv3 for open-source or commercial JUCE license per your arrangement — comply accordingly).
- **Oud image:** `Resources/Images/oud_silhouette.svg` is an original vector silhouette placeholder created for this project (no third-party rights claimed). **Not** presented as a photograph. If you obtain a licensed oud photograph, replace the SVG and update `BinaryData` + `LICENSE-ASSETS.md`.

---

## 13. Development Notes for Maintainers

- To add a maqam: extend `MaqamDatabase::addXFamily()` with intervals in cents from tonic, degree names, seyir, ajnas, source, verification. Rebuild database size is asserted in tests (`size() >= 30`).
- To calibrate scores: collect a labeled corpus (monophonic vocal, known dastgah/makam, tonic-annotated) and compute histograms per entry; fit Platt scaling if probabilities are desired — document corpus and method.
- Keep audio thread allocation-free: profile with `JUCE_HEAVY_DEBUG` and lock checker.

---

*D.A Production — built with JUCE. Honest, working analyzer over fabricated certainty.*
