# Art & Audio Direction: "Dusk Lagoon"

A calm night-water identity: deep teal-to-indigo water, sea-glass tiles, a softly glowing aqua ring.
It deliberately avoids warm beige/clay "generic app" palettes.

## Principles

- Calm over flashy: slow exponential easing, no screen shake, no harsh flashes.
- Few elements: ring, tiles, centre piece, score. Nothing else during play.
- Number legibility first: digits switch between deep ink and white based on tile luminance.

## Palette (source of truth: `Shaders/*.hlsl` for the board, `Source/DoubleIt/Game/DITheme.h` for UI)

| Role | Dark (signature) | Light |
|---|---|---|
| Background gradient | `#0F3B4C → #1B1F4A` | `#CFEFF0 → #E3E6FA` |
| Ring line / glow | `#5FD3C6` + glow | `#3FA9A0` |
| Fullness arc near full | → coral `#FF9B85` | same |
| UI ink / soft ink | `#F2F4F8` / `#8FB3C4` | `#10233A` / `#5A7A8C` |
| Buttons / panel | `#1E4A5C` / `#0C2433` | `#CDE9EC` / `#E3F3F4` |
| Plus / Minus / Breeze | coral `#FF9B85` with dark + / ink `#1C2E4A` with coral rim and − / pearl `#F2F4F8` with wind lines + glow | same |
| Aim guide dots, impact ripples | pearl `#E6EAF2` (dots: coral to the tile a minus would lift; ripples: sun-gold on a first-ever number) | ink `#1C2E4A` instead of pearl, so they show on the light background |

Tile ranks (1, 2, 4, 8… or 1, 2, 3… in Addition) cycle through 12 sea-glass colours:
mint `#7FE0C2`, aqua `#5BC0EB`, lilac `#B9A6F5`, sun `#FFD480`, rose `#F7A8C8`, seafoam `#9EE6E0`,
periwinkle `#8C9EFF`, lime `#C5E86C`, lagoon `#3FB8AF`, orchid `#D98BE0`, pearl `#E6EAF2`, deep sea `#2E86AB`.

Theme follows the phone (Android night mode via JNI, Windows app theme on desktop), with a manual light/dark override in the pause menu.

## Typography

Nunito (OFL, `Content/DoubleIt/Fonts/`): static Regular for text and buttons; the variable file's ExtraLight default for the large score.
Loaded from loose files (staged as NonUFS), so no font assets. Tile digits are procedural, rounded hand-lettered strokes
(arcs and capsules) in the tile shader, tilted a hair per piece; "1" has a flag so it never reads as a sideways minus.

## Tiles: sea-glass pebbles

The aim is organic, not "generic app". Every stone is drawn in `Shaders/DI_Tile.hlsl`:
- Outline: a circle bent by 3 low harmonics with phases seeded per piece, so no two stones match. The phases drift
  slowly over time and the stone swells ~1.5%, so shapes gently morph.
- Shading: soft dome light from the top left, a thin sheen, faint frosted grain, rim darkening like worn glass.
- Breathing: each stone drifts ~1.5 px on its own slow rhythm (`Shaders/DI_TileBreath.hlsl`, vertex shader).
- Aim dots and ripples stay perfect circles and hold still.

## Motion (`ADIRingActor`)

All motion is polar (angle + radius), so nothing cuts across the inside of the ring:
- The ring has no fixed top: after each step it re-anchors so pieces move as little as possible overall. A gap opens
  evenly on both sides of a placed piece, and a fusion collapses onto the plus from left and right at once.
- Pieces are tracked by id: when the ring reflows they slide along the ring arc (shortest way round) to their new angle.
- Moves play back in steps: the piece lands, each chain collision follows ~220 ms apart, then the next centre appears.
  Each collision: result pulses in (1.22×), a thin ripple ring spreads (0.55 s), chains of 3+ add a 2–4 px shake.
- Motion uses lightly under-damped springs (damping ratio ~0.75, ~3% overshoot): a small, soft settle, not a bounce.
- Placing travels straight out along the target spoke; a minus lift travels straight in along the piece's own spoke.
- Fusion: consumed pieces slide along the arc into the merge point and fade, and the result pulses in at 1.22× its size and settles.
- New centre pieces grow from nothing; the background drifts on a ~60 s cycle.
- Aim guide: while a finger is down (or the mouse hovers) a dotted spoke runs from the centre to the landing gap, with a larger dot
  at the gap. The piece is placed when the finger lifts. Settle after a move averages ~0.7 s including chain playback.

## Audio and haptics (all generated, no assets)

- `Core/DISynth`: kalimba-like plucks (soft attack, decay, inharmonic partials) on a C-major pentatonic scale.
- Place: soft low note whose pitch follows where the piece lands (8 notes around the ring), so every move is musical.
- Chain: one note per collision, each higher (step = depth + 1), played as the collision lands (~220 ms apart);
  a quiet low note settles chains of 2+. Breeze: quick high-to-low wind-chime cascade. Minus lift: two falling notes.
- First-ever number (32+ in Classic): bright three-note chime and a sun-gold ripple. Passing your best mid-run:
  soft major chord, and the score swells and glows gold for 1.2 s. Game over: gentle descending three notes.
- Haptics: 8 ms on place and lift, 20–44 ms per chain collision by depth, 24 ms new best, 40 ms game over. Toggles for sound and vibration live in the pause menu.
