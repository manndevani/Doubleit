#include "Misc/AutomationTest.h"
#include "Core/DIAdaptive.h"
#include "Core/DIGame.h"
#include "Core/DIMode.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags DIModeTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** A made-up mode: pairs triple. Proves a mode is one class + one Register call, with no game code changes. */
	class FDITriplingRules final : public IDIRuleSet
	{
	public:
		virtual int64 FirstFusion(int64 Outer) const override { return Outer * 3; }
		virtual int64 ChainFusion(int64 Center, int64 Outer) const override { return Center + Outer * 3; }
		virtual int64 SpawnNumber(int32 Level, FRandomStream& Stream) const override { return 3 + Level; }
		virtual int32 ColourRank(int64 Value) const override { return int32(Value % 12); }
		virtual int64 FusionScore(int64 Result, int32 Depth) const override { return 1; }
	};

	TArray<FDIPiece> Nums(std::initializer_list<int64> Values)
	{
		TArray<FDIPiece> Out;
		for (int64 V : Values)
		{
			Out.Add(FDIPiece::Number(V));
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIModeRegistryTest, "DoubleIt.Modes.Registry", DIModeTestFlags)
bool FDIModeRegistryTest::RunTest(const FString&)
{
	TestTrue(TEXT("Classic built in"), DIModes::Contains(DIModes::Classic));
	TestTrue(TEXT("Addition built in"), DIModes::Contains(DIModes::Addition));
	TestEqual(TEXT("unknown key falls back to Classic"), DIModes::Get(TEXT("NoSuchMode")).Key, FName(DIModes::Classic));
	TestEqual(TEXT("Classic doubles a chain"), DIModes::Get(DIModes::Classic).Rules->ChainFusion(4, 8), int64(32));
	TestEqual(TEXT("Addition adds a chain"), DIModes::Get(DIModes::Addition).Rules->ChainFusion(4, 8), int64(20));
	TestEqual(TEXT("Addition reveals from 6"), DIModes::Get(DIModes::Addition).Config.RevealFrom, int64(6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIModeCustomTest, "DoubleIt.Modes.CustomModePlays", DIModeTestFlags)
bool FDIModeCustomTest::RunTest(const FString&)
{
	FDIModeConfig Config;
	Config.StartRingPieces = 3;
	Config.MaxRingPieces = 4;
	DIModes::Register({ TEXT("TestTripling"), MakeShared<FDITriplingRules>(), Config });
	TestTrue(TEXT("registered"), DIModes::Keys().Contains(FName(TEXT("TestTripling"))));

	FDIGame Fresh(DIModes::Get(TEXT("TestTripling")), 7);
	TestEqual(TEXT("start ring from config"), Fresh.GetRing().Num(), 3);

	FDIGame Game(DIModes::Get(TEXT("TestTripling")), 7);
	Game.SetRingForTest(Nums({ 5, 5, 1 }));
	Game.SetCenterForTest(FDIPiece::Plus());
	Game.PlaceAt(1);
	TestEqual(TEXT("rule set fuses"), Game.GetRing()[0].Value, int64(15));
	TestEqual(TEXT("rule set scores"), Game.GetScore(), int64(1));

	Game.SetRingForTest(Nums({ 1, 2, 4, 8 }));
	Game.SetCenterForTest(FDIPiece::Number(16));
	Game.PlaceAt(0);
	TestTrue(TEXT("max ring from config"), Game.IsGameOver());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIModeConfigTest, "DoubleIt.Modes.ConfigDrivesRules", DIModeTestFlags)
bool FDIModeConfigTest::RunTest(const FString&)
{
	FDIMode Mode = DIModes::Get(DIModes::Classic);
	Mode.Config.BreezeClearFraction = 0.25f;
	FDIGame Game(Mode, 1);
	Game.SetRingForTest(Nums({ 64, 1, 32, 2, 16, 4, 8, 128 }));
	Game.SetCenterForTest(FDIPiece::Breeze());
	Game.PlaceAt(0);
	TestEqual(TEXT("breeze clears a quarter"), Game.GetRing().Num(), 6);

	FDIAdaptiveTuning Bounds;
	Bounds.MaxPlusChance = 0.18f;
	FDIPlayerProfile Struggling;
	for (int32 i = 0; i < 6; ++i)
	{
		DIAdaptive::Learn(Struggling, { 20, 1, 6, 1, 3.f, 8 }, Bounds);
	}
	TestTrue(TEXT("adaptive bounds from config"), DIAdaptive::TuningFor(Struggling, FDISpawnTuning(), Bounds).PlusChance <= 0.18f + 1e-4f);
	return true;
}

#endif
