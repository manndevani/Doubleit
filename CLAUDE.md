# Double It: working rules

- UE 5.8 C++ project. Engine: `C:\Program Files\Epic Games\UE_5.8`. Module: `DoubleIt`.
- **Game rules live in pure C++** under `Source/DoubleIt/Core/` (no UObject, no rendering). Change rules there
  and cover them with a test in `Source/DoubleIt/Tests/`. The spec is `docs/GAME_DESIGN.md`: keep both in sync.
- Blueprints only for UMG widget layouts and asset references. Logic in BP is a bug.
- Materials: use Custom (HLSL) nodes, unlit, opaque/masked where possible, and no per-tile dynamic material instances
  (use per-instance custom data).
- Materials are generated from `Shaders/*.hlsl`: edit the HLSL, then run `./Scripts/generate_assets.ps1`. Never hand-edit `M_*`.
- Visual check: `./Scripts/screenshot.ps1 -Moves 20` gives `Saved/Screenshots/board.png`. Look at one image, not many.
- Unreal Editor must be closed before building from the CLI (Live Coding blocks UBT).
- Verify with `./Scripts/run_tests.ps1` (headless, exit code = pass/fail). Don't ask the user to click through the editor
  to check logic.
- Track progress by ticking boxes in `docs/ROADMAP.md`; log non-obvious choices in `docs/DECISIONS.md`.
