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
| Plus / Minus / Breeze | coral `#FF9B85` / aqua `#5BC0EB` / pearl `#F2F4F8` + glow | same |

Tile ranks (1, 2, 4, 8… or 1, 2, 3… in Addition) cycle through 12 sea-glass colours:
mint `#7FE0C2`, aqua `#5BC0EB`, lilac `#B9A6F5`, sun `#FFD480`, rose `#F7A8C8`, seafoam `#9EE6E0`,
periwinkle `#8C9EFF`, peach `#FFC3A0`, lagoon `#3FB8AF`, orchid `#D98BE0`, pearl `#E6EAF2`, deep sea `#2E86AB`.

Theme follows the phone (Android night mode via JNI, Windows app theme on desktop), with a manual light/dark override in the pause menu.

## Typography

Nunito (OFL, `Content/DoubleIt/Fonts/`): static Regular for text and buttons; the variable file's ExtraLight default for the large score.
Loaded from loose files (staged as NonUFS), so no font assets. Tile digits are procedural seven-segment glyphs in the tile shader.

## Motion (`ADIRingActor`)

- Pieces are tracked by id: when the ring reflows they glide to their new angle.
- Fusion: consumed pieces glide into the merge point and fade, and the result pulses in at 1.35× its size and settles.
- New centre pieces grow from nothing; the background drifts on a ~60 s cycle.

## Audio and haptics (all generated, no assets)

- `Core/DISynth`: kalimba-like plucks (soft attack, decay, inharmonic partials) on a C-major pentatonic scale.
- Place: low soft note. Fusion: one rising note per chain step (120 ms apart). Game over: gentle descending three notes.
- Haptics: 10 ms on place, 18–48 ms on fusion by chain depth, 40 ms on game over. Toggles for sound and vibration live in the pause menu.
