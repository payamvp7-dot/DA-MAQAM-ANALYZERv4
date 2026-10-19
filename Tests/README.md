# Tests — D.A Maqam Analyzer

## Unit tests (optional, build with -DBUILD_TESTS=ON)

The `Tests/` dir contains lightweight JUCE-free logic tests for:
- YIN sine accuracy (±10c tolerance)
- Maqam database size / no-duplicate IDs / tradition counts
- Analyzer: insufficient evidence on silence, tonic estimation on synthetic pitch sets
- Tuning SCL generation

On CI (Ubuntu) they are built and run via `ctest`.

To run locally on Windows:

```pwsh
cmake -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Manual DAW tests must still be done by the user in Studio One — see `TEST_NOTES.md`.
