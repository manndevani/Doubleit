# Architecture

## Layers

```
Source/DoubleIt/
  Core/            pure C++ rules: no UObject, no rendering, 100% unit-tested
    DITypes.h        FDIPiece, EDIPieceKind, EDIRuleSet
    DIRules.*        fusion math per rule set
    DISpawner.*      seeded spawner (FRandomStream)
    DIGame.*         ring state machine: PlaceAt / AbsorbAt / ConvertCenterToPlus
  Game/            UE glue
    DoubleItGameMode.*   owns one FDIGame, exposes BlueprintCallable API + events
  Tests/           automation tests (DoubleIt.Core.*)
  (planned)
  Render/          ADIRingActor: one UInstancedStaticMeshComponent for all tiles
  UI/              UUserWidget C++ bases (BindWidget) for HUD, menus, ads page
  Save/            UDISaveGame: high scores, settings, ad choice
```

## The C++ / Blueprint boundary

| Thing | Where | Why |
|---|---|---|
| Rules, spawning, scoring, save data | C++ | testable headless, deterministic |
| Input → ring slot math (angle → slot) | C++ | pure math, testable |
| Tile rendering and animation curves | C++ (`ADIRingActor`, timers or `FTSTicker`) | no BP tick overhead |
| Widget **layout** (`WBP_*`) | Blueprint (UMG designer) | visual layout only; derives from C++ base classes |
| Maps, data assets (palette, tuning) | Assets | designer-tweakable without recompiling |

## Rendering plan (mobile-optimised)

- Orthographic camera and **unlit** materials, with no dynamic lights, shadows or post-processing (bloom, motion blur and auto exposure are off).
- All ring tiles come from **one** `UInstancedStaticMeshComponent` (one quad mesh), so a draw call covers every tile.
  Per-instance custom data `[value index, colour index, alpha, scale]` drives the material.
- Tile material (`Shaders/DI_Tile.hlsl`): a Custom HLSL node draws an anti-aliased disc (SDF) and **procedural
  seven-segment digits**, so there are no textures, no per-tile `UTextRenderComponent` and no dynamic material instances. Tile space is
  derived from the screen-space UV Jacobian, so glyphs are upright whatever the mesh's UV layout.
- Background (`Shaders/DI_Background.hlsl`): a drifting gradient, the ring line and a fullness arc (custom primitive data 0).
  It reads the camera-relative world position, so **the camera must stay at X=Y=0**.
- Materials are **generated, not hand-made**: `Scripts/generate_assets.ps1` runs `UDIGenerateAssetsCommandlet`, which
  builds `/Game/DoubleIt/Materials/M_*` from the `.hlsl` files. Edit the HLSL, re-run the script and commit the `.uasset`.
- Exact colours: the camera uses manual exposure with `ToneCurveAmount=0` on desktop; mobile uses `r.MobileHDR=False` (no post at all).
- Map: `/Engine/Maps/Entry` (empty). Everything is spawned from C++ by `ADoubleItGameMode`.
- Particles are Niagara GPU sprites, few and soft, used only on fusion.

## Data flow

```
touch → APlayerController → angle → slot index → AGameMode::PlaceAt(slot)
      → FDIGame (rules) → events (fusions, removed indices) → ADIRingActor animates → HUD updates score
```

## Target platform settings (Config/DefaultEngine.ini)

Android arm64 only, minSdk 26, targetSdk 36, Vulkan + ES 3.2, portrait, ASTC textures. Packaged as an AAB for Play.
