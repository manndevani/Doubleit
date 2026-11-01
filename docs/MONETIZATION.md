# Monetization: "Support" page

The player chooses how to support the game. Nothing is forced, and gameplay is never interrupted. The one thing sold
that touches play is the breeze, which can always be earned by playing too.

| Option | What it does | Implementation |
|---|---|---|
| **Gentle banner** | A small banner at the very bottom, outside the play area, shown only on menus and game over (never during play) | AdMob banner via UE `ShowAdBanner(bShowOnBottomOfScreen=true)`; the play area is laid out above the banner's safe height |
| **No ads** | Default. Ads stay off for as long as this is selected; the choice is saved | `UDISaveGame::AdChoice = None` |
| **Tip the developer** | Optional one-off donation tiers (e.g. small / medium / large) | Google Play Billing consumable IAPs via `OnlineSubsystemGooglePlay` |
| **Breeze packs** | 3 or 10 breezes (the aimed half-clear). Free ones still come from play: 1 to start, 1 per first-ever number from 256 | Google Play Billing consumable IAPs (`breeze_3`, `breeze_10`) behind `IDIStore` (`Game/DIStore.h`); a fake store in development builds, "opens soon" in shipping until connected |

## Open questions for the owner

- [x] Is "No ads" the **default** (opt-in ads), or are banners on by default with a free "turn off" switch? **No ads by default; the player opts in.**
- [x] Should the banner show during gameplay or only on menus/game-over? **Menus and game over only, never during play.**
- [ ] What are the tip tiers and prices?
- [x] Breeze pack prices (Atomas sold 5 antimatter for about $2.99): **3 for $0.99, 10 for $2.49** (USD base; Play converts per country)
- [x] Leaderboards: bought breezes help scores. Keep one board, or separate / exclude runs that used a bought breeze? **One board; every run counts.**

## Compliance notes

- Donations must go through **Google Play Billing**; external payment links for in-app tips are not allowed.
- AdMob needs: an AdMob app ID, a **UMP consent dialog** (GDPR/EEA), the Play Console "Contains ads" declaration,
  a Data safety form entry and a privacy policy URL.
- Do not target ads to children. Set the content rating and target audience to 13+ unless we design for kids (Families policy).
