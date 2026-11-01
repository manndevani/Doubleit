# Double It

A calm, minimalist number-merging puzzle for Android, inspired by the ring mechanic of *Atomas*.
Instead of chemical elements, the ring holds **numbers**. Drop a **+** between two equal numbers and they merge;
symmetric numbers around the merge keep chaining outward.

- **Engine:** Unreal Engine 5.8, C++ first (Blueprints only for UMG layouts / asset wiring)
- **Target:** Android (Google Play), portrait, arm64, Vulkan + OpenGL ES 3.2
- **Modes (rule sets):** Doubling (v1 Classic) and Addition (unlockable)

## Docs map

| File | What it's for |
|---|---|
| [CLAUDE.md](CLAUDE.md) | Short rules for AI/dev sessions (read first, keeps sessions cheap) |
| [docs/GAME_DESIGN.md](docs/GAME_DESIGN.md) | Rules, pieces, merge math, spawning, scoring |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Code layout, C++ vs Blueprint boundary, rendering plan |
| [docs/ART_AND_AUDIO.md](docs/ART_AND_AUDIO.md) | Minimal/soothing visual + sound direction, palette |
| [docs/MONETIZATION.md](docs/MONETIZATION.md) | Ads choice page, banner rules, donations |
| [docs/TESTING.md](docs/TESTING.md) | Automated, headless test strategy |
| [docs/SETUP.md](docs/SETUP.md) | Toolchain (UE 5.8, VS 2022, Android SDK/NDK) |
| [docs/PLAYSTORE.md](docs/PLAYSTORE.md) | Release checklist for Google Play |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Milestones + task checkboxes (the tracker) |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Decision log (why we chose what we chose) |
| [docs/reference/ATOMAS_NOTES.md](docs/reference/ATOMAS_NOTES.md) | What we learned from the reference screenshots |

## Quick commands

```powershell
./Scripts/build_editor.ps1      # compile C++ (Win64 editor)
./Scripts/run_tests.ps1         # build + run all DoubleIt.* automation tests headless
./Scripts/generate_assets.ps1   # rebuild materials from Shaders/*.hlsl
./Scripts/screenshot.ps1        # auto-play + save Saved/Screenshots/board.png
./Scripts/package_android.ps1   # cook + package an Android build (needs SDK/NDK, see docs/SETUP.md)
```
