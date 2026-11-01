# Game Design

> Source of truth for rules. Code: `Source/DoubleIt/Core/`. Numbers marked *(tune)* are expected to change after playtests.

## Core loop

1. A **ring** of pieces surrounds a **center** piece.
2. Tap a gap in the ring to place the center piece there. While the finger is down, a dotted line runs from the center to the gap
   it will land in; drag to change the gap and lift to place.
3. Merges resolve, a new center piece spawns, and you repeat.
. Leaving never costs a run: the run in progress is saved after every move and when the app goes to the background,
   and the next launch picks it up exactly where it was (same next pieces, same score). A finished run, or opening the
   tutorial, clears it.

## Pieces

| Piece | Look | Behaviour |
|---|---|---|
| **Number** | soft circle with a number | Placed into the ring. |
| **Plus (+)** | warm accent dot | When both neighbours are equal numbers, it fuses them (see below). If they aren't, it stays in the ring and fires later when its neighbours become equal. |
| **Minus (−)** | dark dot with a coral rim | Tap any ring piece to take it out. That piece becomes the new center; you can place it, or tap the center to turn it into a Plus. |
| **Breeze** | white glow | Never dealt: the player owns breezes and brings one in by choice (see [Breeze](#breeze)). Placed in a gap, it clears the numbers in the half of the ring nearest that gap (`floor(ringSize/2)` pieces outward from the tap, alternating sides). Pluses and minuses there stay. The calm version of Atomas' Antimatter. |

Each kind's behaviour is an `IDIPieceType` (`Core/DIPieceType.h`); the game asks the type (is it a catalyst, does it lift,
what does placing do), so a new special piece does not touch the game logic. The breeze share is per mode (`BreezeClearFraction`).

## Breeze

The breeze is a rescue the player owns and uses by choice, never a random piece (like Atomas' antimatter):

- **Using one:** the breeze button (a pearl dot and the count, top right) brings a breeze into the centre; the current
  centre waits behind it (a lifted piece keeps its turn-into-plus option). Tap a gap to clear the half nearest it, or
  tap the button again to put the breeze back unused. After it is used, the waiting piece is the centre again. It
  counts as a move. One is spent only when it is placed. With none left, the button opens the shop.
- **Free breezes:** a new player starts with 1 (`UDIGameSettings::StartingPieces`). Every time you make a number you
  have never made before in that mode, from `RewardFrom` on (Classic 256, Addition 9), you get one: 256, then 512,
  then 1024... Each number pays once, ever (`FDIWallet`, saved). Tutorial and automation runs neither earn nor spend.
- **Shop:** packs of 3 and 10 (`UDIGameSettings::Packs`, product ids `breeze_3`, `breeze_10`), opened from the button
  at zero or from the pause menu. Prices come from the store. Google Play Billing is not connected yet: development
  builds use a fake store that sells instantly; shipping builds say the shop opens soon. See MONETIZATION.md.
- **Tutorial:** the last step gives one free breeze through the same button, to show how it works.

## Fusion

Fusion starts at a Plus whose left (L) and right (R) neighbours are equal numbers `A`:

- **First fusion:** L, R and the Plus become one number: `F(A) = A + A` (both rule sets).
- **Chain:** while the new tile's two neighbours are equal numbers `O`, they fuse into it too:

| Rule set | Chain value `C(center, O)` |
|---|---|
| **Doubling** (Classic) | `O < center ? center × 2 : O × 4` (stays on the powers-of-two ladder) |
| **Addition** | `center + O + O` |

- The ring wraps around: index 0 and the last index are neighbours.
- A chain stops when fewer than 3 pieces remain or the neighbours differ.
- After any move, every Plus in the ring is re-checked until nothing changes (the order is deterministic, lowest index first).

### Worked examples (all are automated tests)

| Rule | Ring before → center `+` placed at | Result |
|---|---|---|
| Doubling | `[2,2,8]`, slot 1 | `[4,8]` |
| Doubling | `[8,2,2,8,1]`, slot 2 | `[32,1]` (2+2=4, then 8,8 around 4 → 32) |
| Addition | `[5,3,3,5,1]`, slot 2 | `[16,1]` (3+3=6, then 6+5+5) |
| Doubling | `[2,4,2]`, slot 0 (wraps) | `[4,4]` |

**How a fusion is shown:** the rules resolve instantly, then the move plays back step by step. The piece travels out and lands, the
first collision starts a beat (0.14 s) after it reaches the ring, and each next one follows ~340 ms apart with a rising note. Both neighbours close in on the plus from left and right at the same
time, and the result stays where the plus was (the ring re-anchors; there is no fixed top). Details: ARCHITECTURE.md#move-flow-and-playback,
ART_AND_AUDIO.md#motion-adiringactor.

## Scoring

Each fusion adds `resultValue × chainDepth` (the first fusion is depth 1). The run's high score is saved per rule set.

## Spawning *(tune)*

The spawner is deterministic for a given seed, so a seed always replays the same game.

- **Number range:** Doubling spawns `2^r` with `r ∈ [base, base+2]`, where `base = moves / 40`.
  Addition spawns `v ∈ [1+base, 2+base]` (two values only: its merges leave the spawn range quickly, so pairs must stay common).
- **Balance check:** `DoubleIt.Sim.GreedyBalance` plays 20 seeded games per rule set with a greedy bot.
  Latest results: Doubling averages 45 moves and a score of 355; Addition averages 54 moves and a score of 204.
- **Plus:** guaranteed if none of the last 5 spawns was a Plus; otherwise a 17% chance.
- **Minus:** 5% chance after move 10, never twice in a row.
- **Breeze:** never dealt; owned (see [Breeze](#breeze)). Pieces with `IDIPieceType::IsOwned` have no spawn row.
- The values above are the neutral tuning; adaptive pace shifts the plus chance and `40` per player (below).
- These rules are one ordered table (`FDISpawnTuning::Specials`, one row per special piece) rolled against a single
  roll per spawn; rows can be added or tuned per mode in `Config/DefaultGame.ini` (`PlusChance`, `MinusFromMove`, ...).

## Adaptive pace (`Core/DIAdaptive`, invisible, no setting)

Each mode keeps its own player profile, smoothed over runs (newest run weighs 30%):
average moves survived, longest chain, plus yield (fusions per plus placed), think time (recorded only) and highest number ever.

- **Skill** = 0.6 × survival (30→120 moves) + 0.25 × chains (1→5) + 0.15 × plus yield, pulled toward 0.5 until
  there are a few runs (`confidence = runs / (runs + 2)`).
- **Struggling (skill < 0.5):** up to +5% plus chance, growth step up to 50 moves, plus guarantee window 4.
- **Strong (skill > 0.5):** up to −3% plus chance, growth step down to 34 moves.
- Hard bounds: plus chance 14–22%, growth step 34–50. Help is stronger than push: early losses are what make people quit.
- Learns from finished runs, and from abandoned runs of 15+ moves. Bots, screenshots and the tutorial never learn or adapt.
- Sim check (`DoubleIt.Core.AdaptivePace`): greedy bot runs last 62 moves when helped, 45 neutral, 44 pushed.
- Saved in the save game per mode; Android Auto Backup restores it after a reinstall, so a returning player is not re-learned.

## Modes

| Mode | v1? | Notes |
|---|---|---|
| Classic (Doubling) | ✅ | Default. |
| Addition | later | Unlocked by reaching 1,000 points in Classic *(tune)*. |
| Zen / Timed | later | Ideas only. |

Each mode is a rule set + a config, registered by key (`Core/DIMode.h`, see ARCHITECTURE.md#where-numbers-live-keep-code-free-of-magic-numbers).
Classic and Addition share every number except the reveal floor (32 vs 6). Per-mode tuning can be overridden in
`Config/DefaultGame.ini` without a code change.

## Tutorial (`Core/DITutorial`, 6 scripted boards)

Shown on first launch and from the pause menu ("how to play"). The tutorial is strict: each tap is tried on a copy of the
board first, and a move the step does not teach is never played. The instruction swells, the centre stone wobbles and a
soft low note sounds, and the player simply tries again. While the lifted piece waits in the centre (step 5) the aim line
is hidden, since the ring is not the answer. "(otherwise retry)" below means such moves are refused.
A soft coral circle breathes under the tap that completes each step (a gap, the stone to lift, or the centre). It is
found by trying every possible tap on a copy of the board, preferring the centre, then what the centre piece acts
on: gaps for a piece that is placed, stones for one that lifts (a tap in a gap also lifts the nearest stone, but the
hint should show the stone). So it always points at a correct answer even if a board changes. It hides while a move plays, and the
step is judged before the board redraws, so no hint flashes up for the step just finished.

| Step | Board | Done when |
|---|---|---|
| 1 | `[2,4,8,4]`, centre 2 | any placement |
| 2 | `[2,2,8,4]`, centre + | a fusion happened (otherwise retry) |
| 3 | `[8,2,2,8,1,4]`, centre + | a chain of depth ≥ 2 (otherwise retry) |
| 4 | `[2,4,16,4,8]`, centre − | a piece was lifted |
| 5 | lifted 16 in the centre | the centre was turned into a + (otherwise retry) |
| 6 | 16 pieces, centre 2 | a breeze was brought in with the button (it breathes; free here) and placed (placing the 2: retry) |
