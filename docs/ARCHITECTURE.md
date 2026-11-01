| Sounds and haptics (notes, delays, volumes, haptic ms), chord progression, ambience levels | `UDIGameSettings::Cues`, `Progression`, ambience gains | Project Settings > Game > Double It > Audio |
| Voice timbre and ambience loop shape | `DISynth::FVoice`, `DISynth::FAmbience` defaults | edit the struct defaults |
| Soft bodies and status glow | `UDIGameSettings` > Body, Glow | Project Settings > Game > Double It |# Architecture

## Layers

```
Source/DoubleIt/
  Core/            pure C++ rules: no UObject, no rendering, deterministic for a seed, unit-tested
    DITypes.h        FDIPiece (Kind, Value, Id), FDIFusionEvent (ids + RingAfter snapshot)
    DIMode.*         modes: IDIRuleSet (Strategy: fusion, spawn numbers, colour rank, score), FDIModeConfig
                     (every rule number), DIModes registry (Classic, Addition; Register / Get / Keys)
    DIPieceType.*    piece types (Type Object): IsCatalyst, LiftsFromRing, LiftedConvertsTo, InsertsOnPlace, OnPlaced, LandCue;
                     DIPieces registry (Number, Plus, Minus, Breeze)
    DISpawner.*      seeded spawner + FDISpawnTuning (growth step + weighted special-piece table, FDISpawnRow)
    DIGame.*         ring state machine: PlaceAt / AbsorbAt / ConvertCenterToPlus, events, PlacedRing
    DIRingLayout.*   ring geometry + hit testing with a ring offset; Reanchor (symmetric motion); ApplyTap
    DIAdaptive.*     player profile per mode, Learn / Skill / TuningFor (invisible adaptive pace)
    DISynth.*        chord/scale frequencies, glass-voice notes with a baked room, sea and pad ambience loops (PCM)
    DITutorial.*     6 scripted boards, Evaluate -> Continue / Advance / Retry
  Game/            UE glue
    DoubleItGameMode.*            owns FDIGame, tutorial, save, audio, ring actor, HUD; tap/aim; move playback; learning
    DoubleItGameMode_Playthrough  non-shipping full-run bot (-DIPlaythrough)
    DIGameSettings.*  UDeveloperSettings: playback, motion, layout numbers; MakeMode applies INI mode overrides
    DIRingActor.*    draws everything on the board with one instanced quad mesh (tiles, ripples, aim dots)
    DIPlayerController.*  touch press/drag = aim, release = place; mouse hover = aim, click = place; back/Esc = pause
    DIPawn.*         orthographic camera at the origin looking down -Z (+X = screen up)
    DIGameEvents.h   event hub (Observer, UWorldSubsystem): PiecePlaced, FusionStep, ChainSettled, NumberRevealed,
                     NewBest, GameOver, TutorialAdvanced, StateChanged
    DIAudio.*        listens to events, plays named cues on the current chord (synth notes + haptics), ambience loops
    DIText.h         player text from the string table Content/DoubleIt/Text/DoubleIt.csv (DIText::Get)
    DIPlatform.*     night mode + vibrate (Android JNI via DoubleIt_UPL.xml, Windows registry on desktop)
    DISaveGame.*     settings, per-mode bests and FDIProfileRecord map, tutorial flag; SaveVersion + ordered migration steps
    DITheme.h        Dusk Lagoon UI colours (light/dark)
  UI/DIHUDWidget.*   whole UI built in C++ UMG (score, best, pause panel, tutorial line, game over, best shimmer)
  Editor/DIGenerateAssetsCommandlet  builds M_Tile / M_Background from Shaders/*.hlsl
  Tests/           automation tests (see TESTING.md)
  DoubleIt_UPL.xml  Android manifest/Java additions (VIBRATE, allowBackup, night mode + vibrate methods)
```

## The C++ / Blueprint boundary

| Thing | Where | Why |
|---|---|---|
| Rules, spawning, scoring, adaptive pace | C++ Core | testable headless, deterministic |
| Input -> ring slot math (angle, offset -> slot/tile) | C++ Core (`DIRingLayout`) | pure math, testable; bot predicts taps with it |
| Rendering and animation | C++ (`ADIRingActor`, actor Tick) | no BP tick overhead |
| UI layout | C++ UMG (`RebuildWidget` builds the tree) | owner choice: no Widget Blueprints, reviewable in diffs |
| Materials | generated from HLSL by a commandlet | never hand-edited |

## Ring geometry (`DIRingLayout`)

- Board space is 2D, origin at the ring centre, X right, Y up; world = `(P.Y, P.X, Z)`.
- Tile `i` of `N` sits at angle `Offset + 2*pi*i/N`, clockwise from the top. Slot `k` = insert before `Ring[k]`.
- **The ring has no fixed top.** After every step the game mode calls `Reanchor(Before, BeforeOffset, After)`: the offset
  is the circular mean of how far each surviving piece (matched by id) would move, so overall rotation cancels. Result:
  a placed piece lands exactly in the aimed gap with its neighbours parting evenly, a fusion result lands exactly where
  the plus was with both sides closing in, and a lift closes evenly. Tested in `DoubleIt.Layout.ReanchorIsSymmetric`.
- All hit testing (`SlotFromPoint`, `TileFromPoint`, `ApplyTap`) takes the offset, so taps match what is on screen.

## Move flow and playback

```
finger down/drag -> controller -> GameMode::HandleAim -> RingActor::SetAim (dotted spoke to the gap / minus target)
finger up        -> controller -> GameMode::HandleTap -> DIRingLayout::ApplyTap(Game, P, RingOffset)
                 -> FDIGame resolves the whole move instantly (PlacedRing, events with RingAfter, next centre)
                 -> StartPlayback builds steps: Land (PlacedRing) -> one step per fusion event -> Settled (final ring + centre)
                    each step re-anchored from the previous one; RingOffset = final offset immediately
                 -> timer: land now, first collision +160 ms, next collisions +220 ms, settle +140 ms
                    collision step: RingActor::ShowBoard(ring, event) + Impact(ripple, shake 3+) + Events.FusionStep
                    settled step: BoardChanged (bests, learning, Events.StateChanged), Events.GameOver / ChainSettled /
                    NumberRevealed / NewBest; audio and HUD react as listeners
```

- A tap during playback calls `FinishPlayback` first (jumps to the settled board), so input is never lost or misrouted.
- Minus lifts and converts do not play back: they re-anchor and show immediately.
- The tutorial waits `PlaybackSeconds()` + a pause before swapping boards.

## Rendering (mobile-optimised)

- Orthographic camera, **unlit** materials, no dynamic lights, shadows or post (bloom, motion blur, auto exposure off).
- Everything on the board is **one** `UInstancedStaticMeshComponent` of `/Engine/BasicShapes/Plane` quads, so one draw call.
  Per-instance custom data `[value, kind, rank, alpha, seed, deform a, deform b]` drives `M_Tile`. Kinds: 0 number, 1 plus, 2 minus, 3 breeze,
  4 aim dot (value 1 = coral), 5 ripple ring (value 1 = gold reveal); for 4 and 5, rank 1 = light theme (ink).
- `ADIRingActor` animates in **polar space** (angle + radius per visual, keyed by piece id): reflows and merges slide along
  the arc (shortest way round), placements travel out along a spoke, lifts travel in. Consumed pieces slide into the
  result and fade; results pulse in at 1.22x. Motion is spring-driven (k 230, c 23, 2 substeps, ~3% overshoot).
  Ripples grow and fade over 0.55 s; shake (2-4 px) decays quickly. `Tick` returns early once settled, so a still
  board costs no CPU. `IsAnimating()` reports when everything has settled (used by the playthrough bot).
- Tile material (`Shaders/DI_Tile.hlsl`): seeded pebble outline + rounded procedural digits, plus/minus/breeze
  glyphs, aim dots and ripple rings. Tile space comes from the screen-space UV Jacobian, so glyphs stay upright.
  A second custom node (`Shaders/DI_TileBreath.hlsl`) feeds World Position Offset: idle breathing on the GPU.
  All alive motion (squash, shrug, flowing outline, inner sway) is per-pixel from Time + Seed: zero CPU while idle.
  Stone quads are padded by `TileQuadPad` (must equal the shader's `QuadPad`) so the stretch has room.
  Soft bodies: while animating, `Tick` computes each stone.s deform (stretch along its board velocity + impact wobble)
  as a log-stretch pair; the shader samples with its exponential, so stretches add up and always keep the area.
- Background (`Shaders/DI_Background.hlsl`): drifting gradient, status glow, ring line, fullness arc (custom primitive
  data 0 = fill, 1 = dark, 2 = mood (eased fill), 3 = bloom, 4 = gold, 5 = over). `ADIRingActor` eases them in `Tick`:
  fusions bloom it via `Impact`, game over comes from `SyncFromGame`, and it listens to `NewBest` for a gold bloom. It reads absolute world position, so the camera may move.
- Camera fit (`ADIPawn::FitToViewport`, re-run when viewport size or UI scale changes): the ring is centred in the band
  between the top HUD (500 UMG units) and the game-over lines (80% height) by moving the pawn along world X, and the
  ortho width only zooms out from the 820-unit phone layout (squat screens, foldables), capped at 1.6 px per unit.
  The HUD sits inside a `USafeZone` (notches, cutouts).
- Mobile cost: MSAA off (shaders antialias their own edges), `t.MaxFPS=60`, translucent stones over one opaque quad.
- Materials are generated: `Scripts/generate_assets.ps1` runs `UDIGenerateAssetsCommandlet`. Edit the HLSL, re-run, commit the `.uasset`.
- Exact colours: manual exposure with `ToneCurveAmount=0` on desktop; `r.MobileHDR=False` on mobile.
- Map: `/Engine/Maps/Entry` (empty). Everything is spawned from C++ by `ADoubleItGameMode`.

## Adaptive pace and saved data

- `ADoubleItGameMode::StartNewGame` loads the mode's profile (`Save->Profiles[ModeKey]`, "Classic" / "Addition") and
  builds the run's `FDIGame` from `UDIGameSettings::MakeMode(key)` with `Spawn = DIAdaptive::TuningFor(profile, Spawn,
  Adaptive)`; automation runs use the mode's base tuning.
- `NoteMove` collects the run summary (moves, pluses placed, first-step fusions, best chain, think time capped at `Adaptive.MaxThinkSeconds`).
- `LearnFromRun` runs at game over, or when a run of `Adaptive.MinAbandonedMoves` (15) moves or more is abandoned; it updates and saves the profile.
- New modes get a new `ModeKey` and their own profile automatically.
- Save file: UE `SaveGameToSlot("DoubleIt")`. On Android, Auto Backup (`android:allowBackup="true"`) restores it after reinstall.
  Rules and limits are in GAME_DESIGN.md#adaptive-pace and DECISIONS.md.

## Target platform settings (Config/DefaultEngine.ini)

Android arm64 only, minSdk 26, targetSdk 36, Vulkan + ES 3.2, portrait, ASTC textures. Packaged as an AAB for Play.

## Where numbers live (keep code free of magic numbers)

| What | Where | Change it by |
|---|---|---|
| Rules that differ by mode (fusion maths, spawned numbers, colour rank, score) | an `IDIRuleSet` class in `Core/DIMode.cpp` | writing a new rule set class |
| Rule numbers (start ring, ring limit, breeze share, reveal floor, spawn chances, adaptive bounds) | `FDIModeConfig` defaults in `Core/DIMode.h` | `[DoubleIt.Mode.<Key>]` in `Config/DefaultGame.ini` (keys = field names) |
| Feel and layout (playback delays, springs, pulse, ripples, aim dots, camera fit) | `UDIGameSettings` | Project Settings > Game > Double It (saved to `DefaultGame.ini`) |
| Special pieces (what each kind does, its landing cue) | an `IDIPieceType` in `Core/DIPieceType.cpp` | a new type class + `DIPieces::Register` + a spawn row |
| Spawn table (chance, from move, min ring, guarantee window, no repeat) | `FDISpawnTuning::Specials` | `<PieceName>Chance`, `<PieceName>FromMove`... in the mode's INI section |
| Sounds and haptics (notes, delays, volumes, haptic ms) | `UDIGameSettings::Cues` (named cues) | Project Settings > Game > Double It > Audio |
| Player-facing text | `Content/DoubleIt/Text/DoubleIt.csv` string table | add a row, use `DIText::Get(key)`; translate via the Localization Dashboard |
| Shader look (palette, glyphs, pebble shape) | `Shaders/*.hlsl` | edit, then `Scripts/generate_assets.ps1` |

Adding a mode: subclass `IDIRuleSet`, `DIModes::Register({ Key, MakeShared<FMyRules>(), Config })`, then
`StartNewGame(Key, Seed)`. Profile, best score and INI overrides follow the key automatically; nothing switches on a mode.
`DoubleIt.Modes.CustomModePlays` does exactly this with a test-only mode.

## Events (Observer)

The game mode is the controller: it owns the run and drives the board view (`ADIRingActor`) directly. Everything else
that reacts to the game subscribes to `UDIGameEvents` (a world subsystem with typed native delegates), so a new feature
(achievements, analytics, leaderboards, another screen) is a new listener and needs no change to the game mode.
Events fire when a moment is shown to the player, not when Core resolves it, so sound and HUD stay in step with playback.
Fast-forwarded steps send no `FusionStep` / `ChainSettled`.
`RingFilled(fraction)` is sent whenever the settled board is shown (the ambience warms with it) and `PauseChanged`
when the pause menu opens or closes.

## Save versioning

`UDISaveGame::SaveVersion` defaults to 0 on purpose: a save from before versioning has no such field, loads as 0 and
runs every migration step. `Migrate()` runs `Steps[v]` (v -> v+1) up to `CurrentVersion`, then the game re-saves.
New saves are stamped with `MarkNew()`. A save from a newer build is left untouched (warning in the log).
To change the layout: add fields (never remove or rename a shipped one), bump `CurrentVersion`, append one step in
`DISaveGame.cpp` (a `static_assert` checks one step per version), and extend `DoubleIt.Save.Migrations`.
