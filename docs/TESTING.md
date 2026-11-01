# Testing: automated, no manual clicking

## Layers

| Layer | Tool | Runs where | Command |
|---|---|---|---|
| Rules (fusion, chains, spawner, game over) | UE Automation tests (`DoubleIt.Core.*`) | headless editor, `-nullrhi` | `./Scripts/run_tests.ps1` |
| Gameplay glue (GameMode API) | Automation tests (`DoubleIt.Game.*`) | headless | same |
| Simulation / balance | `DoubleIt.Sim.*`: plays N seeded games with a greedy bot and logs average score/moves | headless | `./Scripts/run_tests.ps1 -Filter DoubleIt.Sim` |
| Visual check | `-DIAutoplay=N -DIShot=<png>`: the game plays N bot taps, waits for shaders, saves a PNG and quits | Win64 window | `./Scripts/screenshot.ps1 -Moves 20` |
| Full run and feel | `-DIPlaythrough[=Seed] -DIShotDir=<dir>`: a bot plays one Classic game through real screen taps, checks every tap against `DIRingLayout::ApplyTap`, restarts, logs `Playthrough: RESULT=PASS|FAIL` with settle and frame times, and builds a contact sheet | Win64 window | `./Scripts/playthrough.ps1` |
| Specific boards | `-DITutorialStep=N` opens tutorial board N; `-DIAim=<degrees>` holds the aim guide at that ring angle | Win64 window | `./Scripts/screenshot.ps1 -Moves 0 -Extra '-DITutorialStep=3 -DIAim=200'` |
| Device smoke test | `adb install` + launch + logcat grep for `LogDoubleIt: Ready` | phone/emulator | `./Scripts/package_android.ps1 -Install` |

## Rules

- Every rule in `GAME_DESIGN.md` has a matching test. Worked examples in the doc are copied verbatim into tests.
- Tests use `FDIGame::SetRingForTest/SetCenterForTest` so they never depend on RNG.
- The spawner is tested by seed determinism and the frequency guarantees (e.g. a Plus at least every 6 spawns).
- `run_tests.ps1` exits non-zero on any failure and writes a JSON report to `Saved/TestReport/`.
