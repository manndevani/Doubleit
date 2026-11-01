#include "DIAds.h"
#include "Core/DIAdRules.h"
#include "Game/DIGameEvents.h"
#include "Game/DISaveGame.h"
#include "Game/DoubleItGameMode.h"
#include "Kismet/GameplayStatics.h"

void UDIAds::Init(ADoubleItGameMode& InGameMode, UDISaveGame& InSave, UDIGameEvents& InEvents)
{
	GameMode = &InGameMode;
	Save = &InSave;
	Events = &InEvents;
	Network = DIAdNetworks::Make();
	bOptedInNow = Save->bAdsOn;
	Events->StateChanged.AddUObject(this, &UDIAds::Update);
	Events->PauseChanged.AddWeakLambda(this, [this](bool) { Update(); });
	if (bOptedInNow)
	{
		Gather(false); // consent is checked again every launch; the form only shows when it is needed
	}
}

bool UDIAds::IsOptedIn() const
{
	return bOptedInNow;
}

void UDIAds::SetOptedIn(bool bOn, bool bPersist)
{
	bOptedInNow = bOn;
	if (bPersist)
	{
		Save->bAdsOn = bOn;
		UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	}
	if (bOn)
	{
		Gather(bPersist);
	}
	Update();
	Events->StateChanged.Broadcast();
}

void UDIAds::ShowPrivacyOptions()
{
	Network->ShowPrivacyOptions([WeakThis = TWeakObjectPtr<UDIAds>(this)](FDIAdConsent Result)
	{
		if (UDIAds* Self = WeakThis.Get())
		{
			Self->OnConsent(Result, true);
		}
	});
}

void UDIAds::Gather(bool bPlayerAsked)
{
	Network->Gather([WeakThis = TWeakObjectPtr<UDIAds>(this), bPlayerAsked](FDIAdConsent Result)
	{
		if (UDIAds* Self = WeakThis.Get())
		{
			Self->OnConsent(Result, bPlayerAsked);
		}
	});
}

void UDIAds::OnConsent(const FDIAdConsent& Result, bool bAskedNow)
{
	Consent = Result;
	if (bAskedNow && bOptedInNow && !Consent.bCanRequestAds)
	{
		Events->AdsDeclined.Broadcast(); // the HUD says the banner stays off until they agree
	}
	Update();
	Events->StateChanged.Broadcast();
}

void UDIAds::Update()
{
	const ADoubleItGameMode* GM = GameMode.Get();
	if (!GM || !Network)
	{
		return;
	}
	FDIAdState State;
	State.bOptedIn = bOptedInNow;
	State.bCanRequestAds = Consent.bCanRequestAds;
	State.bTutorial = GM->IsInTutorial();
	State.bMenuOpen = GM->IsBoardPaused();
	State.bGameOver = GM->IsGameOver();
	const bool bWant = DIAdRules::ShowBanner(State);
	if (bWant == bShown)
	{
		return;
	}
	bShown = bWant;
	if (bShown)
	{
		Network->ShowBanner();
	}
	else
	{
		Network->HideBanner();
	}
	Events->StateChanged.Broadcast(); // the HUD's placeholder follows
}
