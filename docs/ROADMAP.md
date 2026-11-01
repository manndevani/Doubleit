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
- [x] `ADIRingActor`: instanced tiles placed on the ring, slot hit-testing from touch angle
- [x] Tile material (Custom node SDF disc + procedural seven-segment digits, no textures)
- [x] Background material (drifting gradient, ring line, fullness arc)
- [x] Materials generated from `Shaders/*.hlsl` by the `DIGenerateAssets` commandlet
- [x] Center piece, Plus/Minus/Breeze visuals (Breeze not yet seen in a screenshot)
- [x] Placement easing (C++ exponential ease)
- [ ] Fusion animation (pieces glide into the merge point + pulse) → M2
- [x] HUD: score, best (canvas prototype)
- [ ] Pause button
- [x] Game over state, tap to restart
- [x] Save high score (`UDISaveGame`)
- [x] Automated screenshot check (`Scripts/screenshot.ps1`)
- [ ] Mouse/touch input verified by hand (automation bypasses the controller)

## M2: Feel & polish
- [x] "Dusk Lagoon" identity: dark + light themes, follows the phone, manual override
- [x] Background gradient drift + glowing ring and fullness arc
- [x] Fusion animation: consumed pieces glide into the merge point, the result pulses (id-tracked visuals)
- [x] UI in C++-built UMG: score, best, pause button, pause/settings panel (no Widget Blueprints)
- [x] Nunito font (OFL) loaded from loose files
- [x] Synthesized pentatonic sounds + haptics (Android JNI), sound/vibration toggles
- [x] Tutorial (6 scripted steps, retry on a wrong move), replayable from the menu
- [x] Balance pass with the greedy `DoubleIt.Sim` bot (Addition spawn range narrowed)
- [x] Android back / Esc toggles pause
- [x] Full-run playthrough bot (`Scripts/playthrough.ps1`): PASS, ~0.6 s settle, ~2.6 ms average frame
- [x] Radial motion (arc reflow/merges, radial place/lift), dotted aim guide (touch: aim while held, place on release)
- [x] Glyph clarity: flagged "1", dark/coral minus, breeze wind lines
- [x] Symmetric ring (re-anchored, no fixed top): fusions collapse from both sides, gaps open evenly
- [x] Step-by-step chain playback with rising notes, ripples, haptics, small shake on 3+ chains
- [x] Placement notes by position, breeze chime, first-ever-number reveal, new-best celebration
- [x] Invisible adaptive pace per mode (`Core/DIAdaptive`, tested), saved with the profile
- [x] Android Auto Backup enabled (save restored after reinstall)
- [ ] Touch aim/release verified by hand on a phone (automation drives taps, not drags)
- [ ] Auto Backup restore verified on a phone (`adb shell bmgr backupnow com.doubleit.game`, uninstall, reinstall)
- [ ] Android-only code (JNI, UPL) compiled and tested on a phone, which needs UE's Android platform support installed
- [ ] Save and restore the current run when the app is backgrounded

## M3: Support page & store
- [ ] (?) Ad defaults confirmed (see MONETIZATION.md)
- [ ] AdMob banner + UMP consent
- [ ] Play Billing tips
- [ ] Store assets, privacy policy, closed test (12 testers / 14 days)
- [ ] Play Games Services saved game for the profile (instant, cross-device; needs a Play Console app id)
- [ ] Evaluate Block Store for the profile (reinstall persistence without sign-in; verify on a device first)
- [ ] Privacy policy states play stats stay on the device and in the user's Google backup
- [ ] Production release 1.0

## Later
- [ ] Addition mode unlock
- [ ] Zen / timed modes
- [ ] Play Games leaderboards
