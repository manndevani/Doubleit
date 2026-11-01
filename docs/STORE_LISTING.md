# Google Play store listing (draft)

> Text to paste into Play Console > Store presence > Main store listing. Limits: name 30, short description 80,
> full description 4000 characters. Do not name other games (e.g. Atomas) in the listing: Play's metadata policy
> treats that as keyword spam / impersonation. Keep claims in sync with the game (GAME_DESIGN.md, MONETIZATION.md).

## App name (29/30)

Double It: Calm Number Puzzle

## Short description (75/80)

A calm number puzzle: merge twins on a ring, chain them outward. No timers.

## Full description

Double It is a quiet little number puzzle for the moments in between.

Numbers sit on a ring. Drop a plus between two equal numbers and they merge into their double. If the numbers on
either side match too, the merge keeps going, rippling outward in a chain. Plan a few moves ahead, line up twins, and
watch one tap fold half the ring together.

When the ring fills up, the run ends. That's it: easy to learn, deep enough to keep you coming back for one more run.

HOW IT PLAYS
• Tap a gap in the ring to place the piece in the centre
• A plus merges equal neighbours, and matching pairs beyond them chain on
• A minus lifts any piece off the ring so you can move it, or turn it into a plus
• Save a breeze for a tight spot: it clears the numbers in the half of the ring nearest your tap

MADE TO BE CALM
• No timers, no energy, no lives, no pop-ups
• Soft, generated sound: every merge plays a note, with the sea in the background
• Gentle vibration you can switch off
• Light and dark themes that follow your phone
• Leave any time: your run is saved after every move

FAIR
• No ads unless you choose to turn them on, and even then only on menus, never while you play
• Breezes are earned by playing; packs are there if you want more
• Works offline

A short, hands-on tutorial shows you everything in under a minute.

## Store settings

| Field | Value |
|---|---|
| App category | Game > Puzzle |
| Tags (pick up to 5 in the Console) | Puzzle, Casual, Relaxing, Number, Minimalist (choose the closest offered) |
| Contains ads | Yes (opt-in banner, AdMob) |
| In-app purchases | Yes (breeze packs) |
| Target audience | 13+ |
| Contact email | [contact email] |
| Privacy policy | [URL where docs/PRIVACY_POLICY.md is hosted] |

## Graphics still to make

- [ ] Icon 512×512 (32-bit PNG, no transparency issues; Play rounds the corners)
- [ ] Feature graphic 1024×500
- [ ] 2–8 phone screenshots (portrait, e.g. 1080×1920); `./Scripts/screenshot.ps1` gives the board at the window size

## Data safety (draft answers; check against Google's AdMob data-disclosure page before submitting)

| Question | Answer |
|---|---|
| Does the app collect or share user data? | Yes, only through the AdMob / UMP SDKs when the player turns ads on |
| Data types (collected and shared, purpose Advertising, Fraud prevention, Analytics) | Device or other IDs (advertising ID); Approximate location (from IP); App interactions; Diagnostics |
| Is collection optional? | Yes: ads are off by default and the player opts in |
| Encrypted in transit? | Yes (the SDKs use HTTPS) |
| Can users request deletion? | The developer holds no user data; ad data is handled by Google |
| Play stats, bests and saves | Stay on the device (and in the user's own Android backup); not collected by the developer |
