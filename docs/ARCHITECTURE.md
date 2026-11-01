# Architecture

## Layers

```
Source/DoubleIt/
  Core/            pure C++ rules: no UObject, no rendering, deterministic for a seed, unit-tested
    DITypes.h        FDIPiece (Kind, Value, Id), FDIFusionEvent (ids + RingAfter snapshot), EDIRuleSet
    DIRules.*        fusion math per rule set (FirstFusion, ChainFusion)
    DISpawner.*      seeded spawner + FDISpawnTuning (plus/minus/breeze chances, growth step)
    DIGame.*         ring state machine: PlaceAt / AbsorbAt / ConvertCenterToPlus, events, PlacedRing
    DIRingLayout.*   ring geometry + hit testing with a ring offset; Reanchor (symmetric motion); ApplyTap
    DIAdaptive.*     player profile per mode, Learn / Skill / TuningFor (invisible adaptive pace)
    DISynth.*        pentatonic note frequencies + kalimba-like pluck PCM
    DITutorial.*     6 scripted boards, Evaluate -> Continue / Advance / Retry
  Game/            UE glue
    DoubleItGameMode.*            owns FDIGame, tutorial, save, audio, ring actor, HUD; tap/aim; move playback; learning
    DoubleItGameMode_Playthrough  non-shipping full-run bot (-DIPlaythrough)
    DIRingActor.*    draws everything on the board with one instanced quad mesh (tiles, ripples, aim dots)
    DIPlayerController.*  touch press/drag = aim, release = place; mouse hover = aim, click = place; back/Esc = pause
    DIPawn.*         orthographic camera at the origin looking down -Z (+X = screen up)
    DIAudio.*        synthesized cues (USoundWaveProcedural) + haptics
    DIPlatform.*     night mode + vibrate (Android JNI via DoubleIt_UPL.xml, Windows registry on desktop)
    DISaveGame.h     settings, bests, tutorial flag, per-mode FDIProfileRecord map
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
                    collision step: RingActor::ShowBoard(ring, event) + Impact(ripple, shake 3+) + Audio::PlayChainStep
                    settled step: BoardChanged (HUD, bests, learning), game over / chain settle / reveal / new best cues
```

- A tap during playback calls `FinishPlayback` first (jumps to the settled board), so input is never lost or misrouted.
- Minus lifts and converts do not play back: they re-anchor and show immediately.
- The tutorial waits `PlaybackSeconds()` + a pause before swapping boards.

## Rendering (mobile-optimised)

- Orthographic camera, **unlit** materials, no dynamic lights, shadows or post (bloom, motion blur, auto exposure off).
- Everything on the board is **one** `UInstancedStaticMeshComponent` of `/Engine/BasicShapes/Plane` quads, so one draw call.
  Per-instance custom data `[value, kind, rank, alpha]` drives `M_Tile`. Kinds: 0 number, 1 plus, 2 minus, 3 breeze,
  4 aim dot (value 1 = coral), 5 ripple ring (value 1 = gold reveal).
- `ADIRingActor` animates in **polar space** (angle + radius per visual, keyed by piece id): reflows and merges slide along
  the arc (shortest way round), placements travel out along a spoke, lifts travel in. Consumed pieces slide into the
  result and fade; results pulse in at 1.35x. Ripples grow and fade over 0.55 s; shake (2-4 px) decays quickly.
  `IsAnimating()` reports when everything has settled (used by the playthrough bot).
- Tile material (`Shaders/DI_Tile.hlsl`): SDF disc + procedural seven-segment digits (flagged "1"), plus/minus/breeze
  glyphs, aim dots and ripple rings. Tile space comes from the screen-space UV Jacobian, so glyphs stay upright.
- Background (`Shaders/DI_Background.hlsl`): drifting gradient, ring line, fullness arc (custom primitive data 0 = fill,
  1 = dark). It reads camera-relative world position, so **the camera must stay at X=Y=0**.
- Materials are generated: `Scripts/generate_assets.ps1` runs `UDIGenerateAssetsCommandlet`. Edit the HLSL, re-run, commit the `.uasset`.
- Exact colours: manual exposure with `ToneCurveAmount=0` on desktop; `r.MobileHDR=False` on mobile.
- Map: `/Engine/Maps/Entry` (empty). Everything is spawned from C++ by `ADoubleItGameMode`.

## Adaptive pace and saved data

- `ADoubleItGameMode::StartNewGame` loads the mode's profile (`Save->Profiles[ModeKey]`, "Classic" / "Addition") and
  builds the run's `FDIGame` with `DIAdaptive::TuningFor(profile)`; automation runs use neutral tuning.
- `NoteMove` collects the run summary (moves, pluses placed, first-step fusions, best chain, think time capped at 30 s).
- `LearnFromRun` runs at game over, or when a 15+ move run is abandoned; it updates and saves the profile.
- New modes get a new `ModeKey` and their own profile automatically.
- Save file: UE `SaveGameToSlot("DoubleIt")`. On Android, Auto Backup (`android:allowBackup="true"`) restores it after reinstall.
  Rules and limits are in GAME_DESIGN.md#adaptive-pace and DECISIONS.md.

## Target platform settings (Config/DefaultEngine.ini)

Android arm64 only, minSdk 26, targetSdk 36, Vulkan + ES 3.2, portrait, ASTC textures. Packaged as an AAB for Play.
