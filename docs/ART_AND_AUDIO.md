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
  round the edge (BendScale 1.6, BendSpeed 1.7) and the stone swells ~1.5%, so the shape visibly flows.
- Alive: each stone, on its own seeded clock, squashes and stretches ~4.5% along a slowly turning axis (area kept), and
  every 5-10 s gives a quick damped shrug (~7%). Its number and sheen sway ~0.035 tile inside the body, like jelly.
  Tunables are named constants at the top of the alive block in `Shaders/DI_Tile.hlsl`.
- Stone quads are 1.25x the stone (`TileQuadPad` in DIRingActor, `QuadPad` in the shader) so the stretch never clips.
- Shading: soft dome light from the top left, a thin sheen, faint frosted grain, rim darkening like worn glass.
- Breathing: each stone drifts ~2.5 px on its own slow rhythm (`Shaders/DI_TileBreath.hlsl`, vertex shader).
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

## Status glow (`Shaders/DI_Background.hlsl`)

A soft radial glow sits behind the board and shows how the run is going, as a tint, never a spotlight:
- Its colour follows the ring fullness along a gradient: lagoon `#3FB8AF` (calm) → periwinkle `#8C9EFF` → sun `#FFD480`
  → coral `#FF9B85` (nearly full). The fullness is eased (MoodRate 1.5/s), so the colour drifts rather than jumps.
- A fusion blooms it (from 0.25 for a single merge to 1 for a long chain, fading at 1.6/s). A first-ever number or a
  new best blooms it sun-gold.
- At game over it sinks to dusk `#715A8F` at about half strength (OverRate 1.2/s) and comes back on a new run.
- It breathes ±6% very slowly. Strength 0.2 on dark, 0.16 on light. Numbers live in `UDIGameSettings` > Glow and as
  named constants at the top of the glow block in the shader.

## Soft bodies (stones react to motion)

`ADIRingActor` feeds each stone a deform (two floats of custom data, an area-keeping log-stretch) while it moves:
- Stones stretch along their path by speed (0.00008 of size per unit/s, at most 12%), so slides, placements and the
  ring re-anchoring all read as soft things moving through water.
- Landing on the ring squashes the stone along its spoke (by arrival speed, at most 14%). A fusion impact squashes the
  stone it hits by 14% and wobbles neighbours within 2.2 tiles by up to 6%, less with distance.
- A wobble rings out at 19 rad/s and dies at 6/s, starting with the squash. All numbers: `UDIGameSettings` > Body.

## Audio and haptics (all generated, no assets)

The rule for sound: subtle, never in the way of the game. Every cue below is data: a named list of notes (step,
volume, delay) plus a haptic length, in `UDIGameSettings::Cues` (Project Settings > Game > Double It > Audio). Audio
plays them in response to game events; placed pieces name their own landing cue.

- Voice (`Core/DISynth::FVoice`): struck glass / soft marimba. Fundamental plus 2x, 3.93x and 6.8x partials that die
  faster the higher they are, an 8 ms mallet attack, left and right detuned 3 cents apart for width, and a small
  baked stereo room (four combs + two all-passes per channel, 1.4 s, 22% mix). 1.9 s per note, 32 kHz stereo.
- Moves become melody: cue steps land on the tones of the current chord, which moves on every 6 placements through
  Cmaj9 → Am9 → Fmaj9 → G6add9 (five tones each, all C major, so any order of moves stays in harmony). The chord is
  read when a cue starts, so an arpeggio never straddles a change. `Progression`, `MovesPerChord` in settings.
- Humanising: every note's volume varies ±10%, and later notes of a cue start up to 12 ms late. The first note always
  plays at once, so taps feel instant. Its own random stream: the game's seeded randomness is untouched.
- Sea (`Core/DISea`, 22.05 kHz stereo): live, never a loop. It is rendered a block at a time on the audio thread from a
  new seed each session. Waves arrive every 5-12 s, each with its own strength, side (left to right), swell (1.6-3.2 s)
  and wash-back (2.5-5 s). A wave swells as a low rumble (220 Hz), opens up to a brighter hiss as it breaks (up to
  2.6 kHz), then pulls back with bubbling foam (noise above 1.5 kHz, fluttering at ~14 Hz). A deep distant wash
  (140 Hz) runs underneath, and a soft limiter rounds off overlapping waves. Sea gain 0.2.
- Pads (22.05 kHz stereo loops of 24 s, generated once): a calm pad on C3 G3 C4 (gain 0.12), and a warm pad (E4 G4 D5,
  gain 0.14) that fades in with the ring fullness, a gentle lift rather than an alarm. Kept low so they sit under the
  sea instead of reading as a tone. Pausing drops the ambience to 40% over 1.5 s. Sound off stops it.
- Volume sliders in the pause menu: effects (default 70%) and sea (default 40%). Live while dragging; saved when the
  finger lifts, and the effects slider plays a sample note at the new level.
- Place: soft low note whose pitch follows where the piece lands (8 notes around the ring), so every move is musical.
- Chain: one note per collision, each higher (step = depth + 1), played as the collision lands (~220 ms apart);
  a quiet low note settles chains of 2+. Breeze: quick high-to-low wind-chime cascade. Minus lift: two falling notes.
- First-ever number (32+ in Classic): bright three-note chime and a sun-gold ripple. Passing your best mid-run:
  soft major chord, and the score swells and glows gold for 1.2 s. Game over: gentle descending three notes.
- Haptics: 8 ms on place and lift, 20–44 ms per chain collision by depth, 24 ms new best, 40 ms game over. Toggles for sound and vibration live in the pause menu.
