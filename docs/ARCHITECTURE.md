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
- Tile material: a Custom HLSL node draws an anti-aliased disc (SDF) and samples a **digit atlas** texture to print the
  number, so there is no per-tile `UTextRenderComponent` and no dynamic material instances.
- The background is one full-screen quad with a Custom-node radial gradient plus a slow noise drift (the soothing motion).
- Particles are Niagara GPU sprites, few and soft, used only on fusion.

## Data flow

```
touch → APlayerController → angle → slot index → AGameMode::PlaceAt(slot)
      → FDIGame (rules) → events (fusions, removed indices) → ADIRingActor animates → HUD updates score
```

## Target platform settings (Config/DefaultEngine.ini)

Android arm64 only, minSdk 26, targetSdk 36, Vulkan + ES 3.2, portrait, ASTC textures. Packaged as an AAB for Play.
