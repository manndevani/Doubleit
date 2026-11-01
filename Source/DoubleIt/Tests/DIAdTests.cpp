#include "Misc/AutomationTest.h"
#include "Core/DIAdRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIAdRulesTest, "DoubleIt.Core.AdBannerRules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDIAdRulesTest::RunTest(const FString&)
{
	FDIAdState S;
	S.bMenuOpen = true;
	S.bCanRequestAds = true;
	TestFalse(TEXT("off by default: nothing even on a menu"), DIAdRules::ShowBanner(S));

	S.bOptedIn = true;
	TestTrue(TEXT("opted in + consent: shows on a menu"), DIAdRules::ShowBanner(S));
	S.bMenuOpen = false;
	TestFalse(TEXT("never during play"), DIAdRules::ShowBanner(S));
	S.bGameOver = true;
	TestTrue(TEXT("shows on game over"), DIAdRules::ShowBanner(S));
	S.bCanRequestAds = false;
	TestFalse(TEXT("no consent: nothing"), DIAdRules::ShowBanner(S));
	S.bCanRequestAds = true;
	S.bMenuOpen = true;
	S.bTutorial = true;
	TestFalse(TEXT("never in the tutorial"), DIAdRules::ShowBanner(S));
	return true;
}

#endif
