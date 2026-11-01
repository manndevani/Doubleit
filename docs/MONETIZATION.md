# Monetization: "Support" page

The player chooses how to support the game. Nothing is forced, and gameplay is never interrupted.

| Option | What it does | Implementation |
|---|---|---|
| **Gentle banner** | A small banner at the very bottom, outside the play area, shown only on menus/game-over (*confirm: also during play?*) | AdMob banner via UE `ShowAdBanner(bShowOnBottomOfScreen=true)`; the play area is laid out above the banner's safe height |
| **No ads** | Default. Ads stay off for as long as this is selected; the choice is saved | `UDISaveGame::AdChoice = None` |
| **Tip the developer** | Optional one-off donation tiers (e.g. small / medium / large) | Google Play Billing consumable IAPs via `OnlineSubsystemGooglePlay` |

## Open questions for the owner

- [ ] Is "No ads" the **default** (opt-in ads), or are banners on by default with a free "turn off" switch?
- [ ] Should the banner show during gameplay or only on menus/game-over?
- [ ] What are the tip tiers and prices?

## Compliance notes

- Donations must go through **Google Play Billing**; external payment links for in-app tips are not allowed.
- AdMob needs: an AdMob app ID, a **UMP consent dialog** (GDPR/EEA), the Play Console "Contains ads" declaration,
  a Data safety form entry and a privacy policy URL.
- Do not target ads to children. Set the content rating and target audience to 13+ unless we design for kids (Families policy).
