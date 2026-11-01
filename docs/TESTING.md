# Testing: automated, no manual clicking

## Layers

| Layer | Tool | Runs where | Command |
|---|---|---|---|
| Rules (fusion, chains, spawner, game over) | UE Automation tests (`DoubleIt.Core.*`) | headless editor, `-nullrhi` | `./Scripts/run_tests.ps1` |
| Gameplay glue (GameMode API) | Automation tests (`DoubleIt.Game.*`) | headless | same |
| Simulation / balance | `DoubleIt.Sim.*`: plays N seeded games with a greedy bot and logs average score/moves | headless | `./Scripts/run_tests.ps1 -Filter DoubleIt.Sim` |
| Device smoke test | `adb install` + launch + logcat grep for `LogDoubleIt: Ready` | phone/emulator | `./Scripts/package_android.ps1 -Install` |

## Rules

- Every rule in `GAME_DESIGN.md` has a matching test. Worked examples in the doc are copied verbatim into tests.
- Tests use `FDIGame::SetRingForTest/SetCenterForTest` so they never depend on RNG.
- The spawner is tested by seed determinism and the frequency guarantees (e.g. a Plus at least every 6 spawns).
- `run_tests.ps1` exits non-zero on any failure and writes a JSON report to `Saved/TestReport/`.
