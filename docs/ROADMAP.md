# Roadmap & tracker

Legend: `[x]` done · `[ ]` todo · `(?)` waiting on the owner's decision

## M0: Foundation
- [x] Reference analysis of the Atomas screenshots (`docs/reference/ATOMAS_NOTES.md`)
- [x] Project docs
- [x] UE 5.8 C++ project skeleton, Android config
- [x] Core rules in pure C++ (`FDIGame`, rules, spawner)
- [x] Automated headless tests + scripts
- [x] Android SDK/NDK installed via `setup_android.ps1`
- [ ] UE Android target platform installed in the Launcher (owner)
- [x] First green run of `run_tests.ps1` (10/10, 2026-11-01)
- [ ] Empty map + first Android package installs on a phone
- [ ] `git init` + remote repo

## M1: Playable prototype (Classic / Doubling)
- [ ] `ADIRingActor`: instanced tiles placed on the ring, slot hit-testing from touch angle
- [ ] Tile material (Custom node SDF disc + digit atlas)
- [ ] Center piece, Plus/Minus/Breeze visuals
- [ ] Fusion and placement animations (C++ easing)
- [ ] HUD: score, best, pause
- [ ] Game over screen, restart
- [ ] Save high score (`UDISaveGame`)

## M2: Feel & polish
- [ ] Palette data asset, light/dark theme
- [ ] Background gradient drift material
- [ ] Sounds (pentatonic plucks), haptics, settings toggles
- [ ] Tutorial (6 steps)
- [ ] Balance pass with the `DoubleIt.Sim` bot

## M3: Support page & store
- [ ] (?) Ad defaults confirmed (see MONETIZATION.md)
- [ ] AdMob banner + UMP consent
- [ ] Play Billing tips
- [ ] Store assets, privacy policy, closed test (12 testers / 14 days)
- [ ] Production release 1.0

## Later
- [ ] Addition mode unlock
- [ ] Zen / timed modes
- [ ] Play Games leaderboards
