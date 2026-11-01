# Google Play release checklist

## Account and listing
- [ ] Google Play Console developer account (one-time $25)
- [ ] App name, short description (80 chars), full description
- [ ] Icon 512×512, feature graphic 1024×500, ≥ 2 phone screenshots
- [ ] Privacy policy URL (required: ads + billing)
- [ ] Content rating questionnaire, target audience (13+), Data safety form, Ads declaration

## Build
- [ ] Package name final (currently `com.doubleit.game`, which cannot change after the first upload)
- [ ] `StoreVersion` incremented on every upload; `VersionDisplayName` semver
- [ ] targetSdk meets the current Play requirement (36 configured)
- [ ] AAB (`bEnableBundle`), arm64-v8a, upload key signing, Play App Signing enrolled
- [ ] Size: base module < 150 MB (target < 60 MB: no starter content, ASTC, strip unused plugins)

## Quality
- [ ] All automation tests green
- [ ] Tested on a low-end device (Mali/Adreno, 3–4 GB RAM): 60 fps, no thermal throttling in 15 min
- [ ] Back button / app pause / resume handled; the run is saved on pause
- [ ] Internal testing track → closed testing (new personal accounts must run a closed test with 12 testers for 14 days) → production
