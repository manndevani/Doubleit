#include "Misc/AutomationTest.h"
#include "Core/DIGame.h"
#include "Core/DISpawner.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags DITestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;


	FString RingStr(const TArray<FDIPiece>& Ring)
	{
		TArray<FString> Parts;
		for (const FDIPiece& P : Ring)
		{
			Parts.Add(P.IsNumber() ? LexToString(P.Value) : (P.Kind == EDIPieceKind::Plus ? TEXT("+") : TEXT("?")));
		}
		return FString::Printf(TEXT("[%s]"), *FString::Join(Parts, TEXT(",")));
	}

	FDIGame Setup(const FDIMode& Mode, std::initializer_list<int64> Ring, const FDIPiece& Center)
	{
		FDIGame Game(Mode, 1);
		Game.SetRingForTest(FDIPiece::Numbers(Ring));
		Game.SetCenterForTest(Center);
		return Game;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIFirstFusionTest, "DoubleIt.Core.FirstFusion", DITestFlags)
bool FDIFirstFusionTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 2, 2, 8 }, FDIPiece::Plus());
	TestTrue(TEXT("placed"), Game.PlaceAt(1));
	TestEqual(TEXT("ring"), RingStr(Game.GetRing()), TEXT("[4,8]"));
	TestEqual(TEXT("score"), Game.GetScore(), int64(4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIDoublingChainTest, "DoubleIt.Core.DoublingChain", DITestFlags)
bool FDIDoublingChainTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 8, 2, 2, 8, 1 }, FDIPiece::Plus());
	Game.PlaceAt(2);
	TestEqual(TEXT("ring"), RingStr(Game.GetRing()), TEXT("[32,1]"));
	TestEqual(TEXT("score"), Game.GetScore(), int64(4 * 1 + 32 * 2));
	TestEqual(TEXT("events"), Game.GetLastEvents().Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIAdditionChainTest, "DoubleIt.Core.AdditionChain", DITestFlags)
bool FDIAdditionChainTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Addition), { 5, 3, 3, 5, 1 }, FDIPiece::Plus());
	Game.PlaceAt(2);
	TestEqual(TEXT("ring"), RingStr(Game.GetRing()), TEXT("[16,1]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIWrapAroundTest, "DoubleIt.Core.WrapAround", DITestFlags)
bool FDIWrapAroundTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 2, 4, 2 }, FDIPiece::Plus());
	Game.PlaceAt(0);
	TestEqual(TEXT("ring"), RingStr(Game.GetRing()), TEXT("[4,4]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIDormantPlusTest, "DoubleIt.Core.DormantPlusFiresLater", DITestFlags)
bool FDIDormantPlusTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 2, 4 }, FDIPiece::Plus());
	Game.PlaceAt(1);
	TestEqual(TEXT("plus waits"), RingStr(Game.GetRing()), TEXT("[2,+,4]"));
	Game.SetCenterForTest(FDIPiece::Number(4));
	Game.PlaceAt(1);
	TestEqual(TEXT("plus fired"), RingStr(Game.GetRing()), TEXT("[2,8]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIMinusTest, "DoubleIt.Core.MinusAndConvert", DITestFlags)
bool FDIMinusTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 2, 4, 8 }, FDIPiece::Minus());
	TestFalse(TEXT("minus cannot be placed"), Game.PlaceAt(0));
	TestTrue(TEXT("absorb"), Game.AbsorbAt(1));
	TestEqual(TEXT("ring"), RingStr(Game.GetRing()), TEXT("[2,8]"));
	TestTrue(TEXT("center is 4"), Game.GetCenter() == FDIPiece::Number(4));
	TestTrue(TEXT("convert"), Game.ConvertCenterToPlus());
	TestTrue(TEXT("center is plus"), Game.GetCenter().Kind == EDIPieceKind::Plus);
	TestFalse(TEXT("convert only once"), Game.ConvertCenterToPlus());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIBreezeTest, "DoubleIt.Core.Breeze", DITestFlags)
bool FDIBreezeTest::RunTest(const FString&)
{
	FDIGame Game = Setup(DIModes::Get(DIModes::Classic), { 8, 1, 4, 2 }, FDIPiece::Breeze());
	Game.PlaceAt(0);
	TestEqual(TEXT("lowest half gone"), RingStr(Game.GetRing()), TEXT("[8,4]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIGameOverTest, "DoubleIt.Core.GameOver", DITestFlags)
bool FDIGameOverTest::RunTest(const FString&)
{
	FDIGame Game(DIModes::Get(DIModes::Addition), 1);
	TArray<FDIPiece> Ring;
	for (int32 i = 1; i <= Game.GetMaxRingPieces(); ++i)
	{
		Ring.Add(FDIPiece::Number(i));
	}
	Game.SetRingForTest(Ring);
	Game.SetCenterForTest(FDIPiece::Number(100));
	TestFalse(TEXT("not over yet"), Game.IsGameOver());
	Game.PlaceAt(0);
	TestTrue(TEXT("over"), Game.IsGameOver());
	TestFalse(TEXT("no moves after game over"), Game.PlaceAt(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDISpawnerTest, "DoubleIt.Core.SpawnerDeterministicAndFair", DITestFlags)
bool FDISpawnerTest::RunTest(const FString&)
{
	FDISpawner A(DIModes::Get(DIModes::Classic).Rules.ToSharedRef(), 42);
	FDISpawner B(DIModes::Get(DIModes::Classic).Rules.ToSharedRef(), 42);
	int32 SincePlus = 0;
	int32 MaxGap = 0;
	for (int32 i = 0; i < 500; ++i)
	{
		const FDIPiece PA = A.Next(i, 10);
		const FDIPiece PB = B.Next(i, 10);
		if (!(PA == PB))
		{
			AddError(FString::Printf(TEXT("diverged at %d"), i));
			break;
		}
		SincePlus = PA.Kind == EDIPieceKind::Plus ? 0 : SincePlus + 1;
		MaxGap = FMath::Max(MaxGap, SincePlus);
		if (PA.IsNumber() && !FMath::IsPowerOfTwo(PA.Value))
		{
			AddError(FString::Printf(TEXT("non power of two %lld"), PA.Value));
		}
	}
	TestTrue(TEXT("plus at least every 6 spawns"), MaxGap <= 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDISimulationTest, "DoubleIt.Sim.RandomPlayTerminates", DITestFlags)
bool FDISimulationTest::RunTest(const FString&)
{
	for (int32 Seed = 0; Seed < 50; ++Seed)
	{
		FDIGame Game(Seed % 2 ? DIModes::Get(DIModes::Addition) : DIModes::Get(DIModes::Classic), Seed);
		FRandomStream Bot(Seed);
		int32 Guard = 0;
		while (!Game.IsGameOver() && ++Guard < 5000)
		{
			if (Game.GetCenter().Kind == EDIPieceKind::Minus)
			{
				Game.AbsorbAt(Bot.RandRange(0, Game.GetRing().Num() - 1));
				continue;
			}
			Game.PlaceAt(Bot.RandRange(0, Game.GetRing().Num()));
		}
		if (Guard >= 5000)
		{
			AddError(FString::Printf(TEXT("seed %d did not finish"), Seed));
		}
	}
	return true;
}

#endif
