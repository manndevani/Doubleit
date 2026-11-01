#pragma once

#include "CoreMinimal.h"

/** When the optional banner may show. Spec: docs/MONETIZATION.md (opt-in, menus and game over only, never in play). */
struct FDIAdState
{
	bool bOptedIn = false;      // the player turned the banner on (off by default)
	bool bCanRequestAds = false; // consent gathered where the law needs it (Google UMP)
	bool bTutorial = false;
	bool bMenuOpen = false;     // pause, shop or support panel
	bool bGameOver = false;
};

namespace DIAdRules
{
	inline bool ShowBanner(const FDIAdState& S)
	{
		return S.bOptedIn && S.bCanRequestAds && !S.bTutorial && (S.bMenuOpen || S.bGameOver);
	}
}
