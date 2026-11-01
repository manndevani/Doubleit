#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Game/DIAdNetwork.h"
#include "DIAds.generated.h"

class ADoubleItGameMode;
class UDIGameEvents;
class UDISaveGame;

/**
 * The optional banner: off until the player turns it on in Support. Then consent is gathered (Google UMP, each launch),
 * and the banner shows only while a menu is open or on the game-over screen (DIAdRules), never during play or the
 * tutorial. Reacts to UDIGameEvents only. Spec: docs/MONETIZATION.md
 */
UCLASS()
class DOUBLEIT_API UDIAds : public UObject
{
	GENERATED_BODY()

public:
	void Init(ADoubleItGameMode& InGameMode, UDISaveGame& InSave, UDIGameEvents& InEvents);

	bool IsOptedIn() const;
	/** Turning it on asks for consent first; bPersist false (automation) leaves the save alone. */
	void SetOptedIn(bool bOn, bool bPersist = true);

	/** The player must be able to change their consent (a "privacy choices" button in Support). */
	bool IsPrivacyRequired() const { return Consent.bPrivacyOptionsRequired; }
	void ShowPrivacyOptions();

	/** Desktop development: the HUD draws a placeholder bar where the banner is. */
	bool IsPreviewShown() const { return bShown && Network.IsValid() && Network->IsPreview(); }

private:
	/** bPlayerAsked: the player just turned ads on (not the silent check at launch), so a refusal is worth saying. */
	void Gather(bool bPlayerAsked);
	void OnConsent(const FDIAdConsent& Result, bool bAskedNow);
	void Update();

	TWeakObjectPtr<ADoubleItGameMode> GameMode;
	UPROPERTY() TObjectPtr<UDISaveGame> Save;
	UPROPERTY() TObjectPtr<UDIGameEvents> Events;
	TSharedPtr<IDIAdNetwork> Network;
	FDIAdConsent Consent;
	bool bOptedInNow = false; // this session's choice (equals the save unless automation set it)
	bool bShown = false;
};
