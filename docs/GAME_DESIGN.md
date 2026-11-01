# Game Design

> Source of truth for rules. Code: `Source/DoubleIt/Core/`. Numbers marked *(tune)* are expected to change after playtests.

## Core loop

1. A **ring** of pieces surrounds a **center** piece.
2. Tap a gap in the ring to place the center piece there.
3. Merges resolve, a new center piece spawns, and you repeat.
4. The game ends when the ring holds more than **18** pieces after a move resolves *(tune)*.

## Pieces

| Piece | Look | Behaviour |
|---|---|---|
| **Number** | soft circle with a number | Placed into the ring. |
| **Plus (+)** | warm accent dot | When both neighbours are equal numbers, it fuses them (see below). If they aren't, it stays in the ring and fires later when its neighbours become equal. |
| **Minus (−)** | cool accent dot | Tap any ring piece to take it out. That piece becomes the new center; you can place it, or tap the center to turn it into a Plus. |
| **Breeze** | white glow | Placed anywhere: it clears the lowest half of the number tiles (`floor(ringSize/2)`). Pluses stay. It is the calm version of Atomas' Antimatter. |

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
- **Breeze:** 1.5% chance after move 30, and only while the ring holds at least 12 pieces.

## Modes

| Mode | v1? | Notes |
|---|---|---|
| Classic (Doubling) | ✅ | Default. |
| Addition | later | Unlocked by reaching 1,000 points in Classic *(tune)*. |
| Zen / Timed | later | Ideas only. |

## Tutorial (`Core/DITutorial`, 6 scripted boards)

Shown on first launch and from the pause menu ("how to play"). A wrong move replays the step after a short pause.

| Step | Board | Done when |
|---|---|---|
| 1 | `[2,4,8,4]`, centre 2 | any placement |
| 2 | `[2,2,8,4]`, centre + | a fusion happened (otherwise retry) |
| 3 | `[8,2,2,8,1,4]`, centre + | a chain of depth ≥ 2 (otherwise retry) |
| 4 | `[2,4,16,4,8]`, centre − | a piece was lifted |
| 5 | lifted 16 in the centre | the centre was turned into a + (otherwise retry) |
| 6 | 16 pieces, centre Breeze | any placement |
