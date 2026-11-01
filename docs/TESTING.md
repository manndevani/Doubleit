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
| Glyphs | `-DIGlyphBoard` fills the ring with every glyph (digits, plus, minus, breeze) for a close look | Win64 window | `./Scripts/screenshot.ps1 -Moves 0 -Extra '-DIGlyphBoard'` |
| Screen sizes | Runs the visual check at 20:9, 16:9, 360x640, 3:4 tablet and an unfolded foldable, then builds one sheet (`Saved/Screenshots/sizes.png`); check that the HUD and ring never overlap | Win64 window | `./Scripts/screen_sizes.ps1` |
| Device smoke test | `adb install` + launch + logcat grep for `LogDoubleIt: Ready` | phone/emulator | `./Scripts/package_android.ps1 -Install` |
| Reinstall keeps the profile | Auto Backup round trip: play a run, force a backup, uninstall, reinstall, check logcat `Run tuning: ... (N runs learned)` with N > 0 | phone | `adb shell bmgr backupnow com.doubleit.game`, then `adb uninstall com.doubleit.game` and reinstall |

## Test inventory (`./Scripts/run_tests.ps1`)

| Test | Covers |
|---|---|
| `DoubleIt.Core.FirstFusion`, `DoublingChain`, `AdditionChain`, `WrapAround`, `DormantPlusFiresLater` | fusion rules and chains (GAME_DESIGN.md#fusion) |
| `DoubleIt.Core.MinusAndConvert`, `Breeze`, `GameOver` | special pieces and the end condition |
| `DoubleIt.Core.SpawnerDeterministicAndFair` | seed determinism, plus guarantee, minus/breeze gating |
| `DoubleIt.Core.FusionEventsTrackIds` | event ids the renderer follows |
| `DoubleIt.Core.Synth` | pentatonic and chord frequencies; note PCM (no clipping, decays, stereo width, no clicks at either end); the pad loop has the right length and no seam; the live sea continues seamlessly across blocks, never clips, rises and falls with waves (logs `STAT sea rms`) and differs per seed |
| `DoubleIt.Core.TutorialHappyPathAndRetry` | all 6 tutorial boards, retry on a wrong move |
| `DoubleIt.Core.AdaptivePace` | neutral with no data, struggling/strong detection, bounds, one bad run is only a nudge, sim: helped runs last longer |
| `DoubleIt.Layout.TilePositionsClockwiseFromTop`, `SlotAndTileHitTesting`, `TapRouting` | ring geometry and tap routing |
| `DoubleIt.Layout.ReanchorIsSymmetric` | placed piece lands in the aimed gap, fusion result lands where the plus was, offset hit testing |
| `DoubleIt.Sim.GreedyBalance`, `RandomPlayTerminates` | balance stats (`STAT` lines) and that random play always ends |
| `DoubleIt.Modes.Registry`, `CustomModePlays`, `ConfigDrivesRules` | built-in modes, fallback key, a test-only mode registered and played with no game code changes, config fields (start ring, ring limit, breeze share, adaptive bounds) drive the rules |
| `DoubleIt.Save.Migrations` | a pre-versioning save migrates (bests move to the per-mode map, higher wins; v2 volume sliders get their defaults), re-running is a no-op, fresh saves need nothing, newer saves are left alone (expected warning) |

Latest: 23 passed (1 with an expected warning). Playthrough: PASS, 112 moves, 0 mismatches, settle ~0.7 s average including chain playback.

## Rules

- Every rule in `GAME_DESIGN.md` has a matching test. Worked examples in the doc are copied verbatim into tests.
- Tests use `FDIGame::SetRingForTest/SetCenterForTest` so they never depend on RNG.
- The spawner is tested by seed determinism and the frequency guarantees (e.g. a Plus at least every 6 spawns).
- `run_tests.ps1` exits non-zero on any failure and writes a JSON report to `Saved/TestReport/`.
