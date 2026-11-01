# Decision log

| Date | Decision | Why |
|---|---|---|
| 2026-11-01 | Unreal Engine 5.8 + C++, Blueprints only where needed (UMG layout, asset refs) | Owner's choice; C++ keeps logic testable and fast |
| 2026-11-01 | Materials use Custom (HLSL) nodes, unlit, instanced tiles | Mobile performance: one draw call for the ring |
| 2026-11-01 | Two rule sets: Doubling (Classic, v1) and Addition (later mode) | Owner chose "both as modes" |
| 2026-11-01 | v1 = Classic only | Ship small, then iterate |
| 2026-11-01 | Ads via an opt-in "Support" page (banner / no ads / tip) | Keep the game soothing; the defaults are still open (see MONETIZATION.md) |
| 2026-11-01 | Rules in a pure C++ `Core/` layer with headless automation tests | Verification without manual play; cheap iteration |
| 2026-11-01 | Package name `com.doubleit.game` (placeholder) | Must be finalised before the first Play upload |
| 2026-11-01 | Doubling chain rule `O < C ? 2C : 4O` | Keeps values on the power-of-two ladder; mirrors Atomas' "center+1 / outer+2" |
