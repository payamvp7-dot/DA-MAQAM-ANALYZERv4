# Installation Guide

## Windows VST3 (Studio One 5/6/7, any VST3 host)

1. Ensure **Windows 10 or 11, 64-bit**. The plug-in requires the Visual C++ 2022 Redistributable (usually already installed).
2. Get `DA_MAQAM_ANALYZER_Windows_x64_VST3.zip`:
   - From GitHub Actions: **Actions → Build D.A Maqam Analyzer → latest successful run → Artifacts → DA_MAQAM_ANALYZER_Windows_x64_VST3**
   - Or build locally: `build/DA_Maqam_Analyzer_artefacts/Release/VST3/D.A Maqam Analyzer.vst3`
3. Unzip. You will have a single **bundle** (`*.vst3` is a directory on Windows).
4. Copy it to:
   ```
   C:\Program Files\Common Files\VST3\
   ```
   Windows requires Administrator permission — accept the UAC prompt.
5. Launch or **Restart** Studio One.
6. Studio One should auto-scan. Verify:
   - **Studio One → Browser → Effects → D.A Production → D.A Maqam Analyzer**
7. If missing:
   - **Studio One → Options/Settings → Locations → VST Plug-ins**
   - Ensure `C:\Program Files\Common Files\VST3\` is in the list
   - Click **Reset Blocklist** and **Scan**
   - Check **Help → View Messages** for scan errors
8. Insert on a **mono or stereo** track (pass-through). Use on **dry monophonic vocal** only.

## Uninstall
Delete the bundle:
```
C:\Program Files\Common Files\VST3\D.A Maqam Analyzer.vst3
```
Rescan in Studio One.

## Standalone (testing without DAW)
```
build/DA_Maqam_Analyzer_artefacts/Release/Standalone/D.A Maqam Analyzer.exe
```
Select input device (microphone) → test LEARN. The Standalone uses the same engine.

## Sample Rate / Block Size
Supported: 44.1, 48, 88.2, 96 kHz; block sizes 32–2048. Larger blocks reduce CPU slightly. Analyzer is low-CPU (~2–4% on modern i5).

## Troubleshooting
- **No sound:** The plug-in is pass-through by design; check it is not bypassed and track is not muted.
- **No pitch / 0 voiced frames:** Sing louder, disable gate on channel, check input routing, ensure mono vocal (not master bus).
- **All candidates low similarity (<0.35):** Material is polyphonic, too short, or outside database — try longer phrase with clear finalis.
- **UI overlaps:** Update graphics driver; the plug-in is DPI-aware — try Studio One HiDPI settings.
- **VST3 fails to load:** Ensure 64-bit host (Studio One x64), correct path, and Windows not blocking the file (right-click ZIP → Unblock before extracting if downloaded).

## Building
See `README.md` §4 and `.github/workflows/build.yml`.
