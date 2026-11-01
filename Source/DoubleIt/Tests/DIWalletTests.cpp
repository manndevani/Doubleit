#include "Misc/AutomationTest.h"
#include "Core/DIMode.h"
#include "Core/DIWallet.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIWalletTest, "DoubleIt.Core.WalletRewards",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDIWalletTest::RunTest(const FString&)
{
	const FDIModeConfig& Classic = DIModes::Get(DIModes::Classic).Config;
	const FName Breeze = Classic.RewardPiece;
	FDIWallet Wallet;
	Wallet.Add(Breeze, 1);
	TestEqual(TEXT("starts with one"), Wallet.Count(Breeze), 1);

	TestFalse(TEXT("below the threshold pays nothing"), Wallet.Reached(DIModes::Classic, 128, Classic));
	TestTrue(TEXT("first 256 pays"), Wallet.Reached(DIModes::Classic, 256, Classic));
	TestFalse(TEXT("256 again pays nothing"), Wallet.Reached(DIModes::Classic, 256, Classic));
	TestFalse(TEXT("a lower number pays nothing"), Wallet.Reached(DIModes::Classic, 128, Classic));
	TestTrue(TEXT("first 512 pays"), Wallet.Reached(DIModes::Classic, 512, Classic));
	TestEqual(TEXT("two earned"), Wallet.Count(Breeze), 3);

	// Each mode keeps its own record and its own threshold.
	const FDIModeConfig& Addition = DIModes::Get(DIModes::Addition).Config;
	TestFalse(TEXT("Addition: 8 is below its threshold"), Wallet.Reached(DIModes::Addition, 8, Addition));
	TestTrue(TEXT("Addition: first 9 pays"), Wallet.Reached(DIModes::Addition, 9, Addition));

	TestTrue(TEXT("spend"), Wallet.Spend(Breeze));
	Wallet.Add(Breeze, -10);
	TestEqual(TEXT("never below zero"), Wallet.Count(Breeze), 0);
	TestFalse(TEXT("nothing to spend"), Wallet.Spend(Breeze));
	return true;
}

#endif
