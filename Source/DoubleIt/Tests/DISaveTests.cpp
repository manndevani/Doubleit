#include "Misc/AutomationTest.h"
#include "Game/DISaveGame.h"
#include "Game/DIGameSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDISaveMigrationTest, "DoubleIt.Save.Migrations",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDISaveMigrationTest::RunTest(const FString&)
{
	// A save from before versioning: no version field (loads as 0), bests in the old per-mode fields.
	UDISaveGame* Old = NewObject<UDISaveGame>();
	Old->BestDoubling = 1152;
	Old->BestAddition = 40;
	Old->Bests.Add(DIModes::Classic, 900);
	FDIProfileRecord Played;
	Played.HighestEver = 512;
	Old->Profiles.Add(DIModes::Classic, Played);
	TestTrue(TEXT("migrations ran"), Old->Migrate());
	TestEqual(TEXT("stamped current"), Old->SaveVersion, UDISaveGame::CurrentVersion);
	TestEqual(TEXT("classic best kept (higher wins)"), Old->Bests.FindRef(DIModes::Classic), int64(1152));
	TestEqual(TEXT("addition best moved"), Old->Bests.FindRef(DIModes::Addition), int64(40));
	TestEqual(TEXT("old field cleared"), Old->BestDoubling, int64(0));
	TestEqual(TEXT("volume sliders get their defaults"), Old->AmbienceVolume, UDIGameSettings::Get().DefaultAmbienceVolume);
	TestEqual(TEXT("v4: starts with the new-player breezes"), Old->Owned.FindRef(TEXT("Breeze")), UDIGameSettings::Get().StartingPieces.FindRef(TEXT("Breeze")));
	TestEqual(TEXT("v4: numbers already made never pay"), Old->RewardedUpTo.FindRef(DIModes::Classic), int64(512));
	TestFalse(TEXT("second run is a no-op"), Old->Migrate());

	UDISaveGame* Fresh = NewObject<UDISaveGame>();
	Fresh->MarkNew();
	TestTrue(TEXT("fresh save has volumes"), Fresh->EffectsVolume >= 0.f);
	TestEqual(TEXT("fresh save has its breeze"), Fresh->Owned.FindRef(TEXT("Breeze")), UDIGameSettings::Get().StartingPieces.FindRef(TEXT("Breeze")));
	TestFalse(TEXT("fresh save needs nothing"), Fresh->Migrate());

	UDISaveGame* Future = NewObject<UDISaveGame>();
	Future->SaveVersion = UDISaveGame::CurrentVersion + 1;
	TestFalse(TEXT("newer save is left alone"), Future->Migrate());
	return true;
}

#endif
