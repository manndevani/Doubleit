# Art & Audio Direction: minimal and soothing

## Principles

- Calm over flashy: slow easing (≥ 250 ms, ease-in-out), no screen shake, no harsh flashes.
- Few elements: a ring, tiles, a center piece and the score. Nothing else during play.
- Number legibility first: high contrast between digits and tile, rounded geometric font.

## Palette (draft; final values go in `DA_Palette` data asset)

| Role | Light theme | Dark theme |
|---|---|---|
| Background gradient | `#F4EFE9 → #E6DED6` | `#1E2330 → #141822` |
| Ring line | `#CFC6BC` | `#3A4152` |
| Plus | `#E8A87C` (peach) | same |
| Minus | `#85A9C9` (mist blue) | same |
| Breeze | `#FFFFFF` with soft glow | same |
| Number tiles | hue steps around a muted wheel (sage, sand, clay, lavender…), one per ladder rank | same, brighter |

## Typography

One rounded sans font (e.g. *Nunito* or *Quicksand*, both OFL-licensed). It is baked into the digit atlas for tiles and used
directly in UMG.

## Motion

- Placement: the tile eases in from the center (300 ms).
- Fusion: neighbours glide into the merge point, then a soft scale pulse and ring ripple.
- The background gradient drifts very slowly (60 s cycle).

## Audio

- Soft marimba/kalimba plucks; pitch rises with chain depth (pentatonic, so it never clashes).
- Optional ambient pad loop (toggle in settings).
- Haptics: a light tick on placement and a slightly longer one on fusion (toggle).
- Assets must be royalty-free/CC0; record the source of each one in `docs/DECISIONS.md`.
