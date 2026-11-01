#pragma once

#include "CoreMinimal.h"

/** What the consent step allows (Google UMP on Android). */
struct FDIAdConsent
{
	bool bCanRequestAds = false;
	bool bPrivacyOptionsRequired = false; // the player must be able to change their choice (a button in Support)
};

/**
 * Where the banner comes from (Strategy). UDIAds only talks to this. DIAdNetworks::Make picks: AdMob with Google's
 * consent form on Android (Java in DoubleIt_UPL.xml), a placeholder bar on desktop development builds (the HUD draws
 * it, so the layout can be checked), and nothing on other shipping builds. See docs/MONETIZATION.md.
 */
class IDIAdNetwork
{
public:
	virtual ~IDIAdNetwork() = default;

	/** Gathers consent where required (shows Google's form only if needed), then starts the ad SDK. */
	virtual void Gather(TFunction<void(FDIAdConsent)> Done) = 0;

	/** Shows Google's privacy options form, so the player can change their consent. */
	virtual void ShowPrivacyOptions(TFunction<void(FDIAdConsent)> Done) = 0;

	virtual void ShowBanner() = 0;
	virtual void HideBanner() = 0;

	/** No real ad: the HUD draws a placeholder where the banner would be. */
	virtual bool IsPreview() const { return false; }
};

namespace DIAdNetworks
{
	TSharedRef<IDIAdNetwork> Make();
}
