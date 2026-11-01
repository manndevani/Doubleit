# Double It: working rules

- UE 5.8 C++ project. Engine: `C:\Program Files\Epic Games\UE_5.8`. Module: `DoubleIt`.
- **Game rules live in pure C++** under `Source/DoubleIt/Core/` (no UObject, no rendering). Change rules there
  and cover them with a test in `Source/DoubleIt/Tests/`. The spec is `docs/GAME_DESIGN.md`: keep both in sync.
- No Blueprints so far: UI is UMG built in C++ (`UI/DIHUDWidget`), the map is `/Engine/Maps/Entry`. Logic in BP is a bug.
- **No magic numbers, no mode switches.** Mode-specific maths goes in an `IDIRuleSet`; rule numbers in `FDIModeConfig`
  (`Core/DIMode.h`); feel/layout numbers in `UDIGameSettings`. Only true constants (mesh size, shader kind ids) stay
  `constexpr`. Prefer known patterns (Strategy, Registry, data-driven config) over branching. See ARCHITECTURE.md.
- Materials: use Custom (HLSL) nodes, unlit, opaque/masked where possible, and no per-tile dynamic material instances
  (use per-instance custom data).
- Materials are generated from `Shaders/*.hlsl`: edit the HLSL, then run `./Scripts/generate_assets.ps1`. Never hand-edit `M_*`.
- Visual check: `./Scripts/screenshot.ps1 -Moves 20` gives `Saved/Screenshots/board.png`. Look at one image, not many.
- Unreal Editor must be closed before building from the CLI (Live Coding blocks UBT).
- Verify with `./Scripts/run_tests.ps1` (headless, exit code = pass/fail). Don't ask the user to click through the editor
  to check logic.
- Track progress by ticking boxes in `docs/ROADMAP.md`; log non-obvious choices in `docs/DECISIONS.md`.
- **Every change lands in the docs in the same commit.** Rules/tuning -> `docs/GAME_DESIGN.md`; code structure, data flow,
  new files -> `docs/ARCHITECTURE.md`; visuals, motion, sounds, haptics -> `docs/ART_AND_AUDIO.md`; tests, automation
  flags -> `docs/TESTING.md`; why we chose something -> `docs/DECISIONS.md`; progress -> `docs/ROADMAP.md`.
  Before committing, check that no doc still describes the old behaviour.
