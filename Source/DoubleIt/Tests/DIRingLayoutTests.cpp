#include "Misc/AutomationTest.h"
#include "Core/DIGame.h"
#include "Core/DIRingLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags DILayoutTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	FVector2D AtAngleDeg(float Degrees, float Radius = DIRingLayout::RingRadius)
	{
		const float Rad = FMath::DegreesToRadians(Degrees);
		return FVector2D(Radius * FMath::Sin(Rad), Radius * FMath::Cos(Rad));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDILayoutPositionsTest, "DoubleIt.Layout.TilePositionsClockwiseFromTop", DILayoutTestFlags)
bool FDILayoutPositionsTest::RunTest(const FString&)
{
	TestTrue(TEXT("tile 0 at top"), DIRingLayout::TilePosition(0, 4).Equals(FVector2D(0, 300), 0.01));
	TestTrue(TEXT("tile 1 at right"), DIRingLayout::TilePosition(1, 4).Equals(FVector2D(300, 0), 0.01));
	TestTrue(TEXT("tile 2 at bottom"), DIRingLayout::TilePosition(2, 4).Equals(FVector2D(0, -300), 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDILayoutSlotsTest, "DoubleIt.Layout.SlotAndTileHitTesting", DILayoutTestFlags)
bool FDILayoutSlotsTest::RunTest(const FString&)
{
	TestEqual(TEXT("empty ring"), DIRingLayout::SlotFromPoint(AtAngleDeg(123), 0), 0);
	TestEqual(TEXT("between tile 0 and 1"), DIRingLayout::SlotFromPoint(AtAngleDeg(45), 4), 1);
	TestEqual(TEXT("between tile 2 and 3"), DIRingLayout::SlotFromPoint(AtAngleDeg(225), 4), 3);
	TestEqual(TEXT("between last and first appends"), DIRingLayout::SlotFromPoint(AtAngleDeg(350), 4), 4);
	TestEqual(TEXT("nearest tile"), DIRingLayout::TileFromPoint(AtAngleDeg(80), 4), 1);
	TestEqual(TEXT("nearest tile wraps"), DIRingLayout::TileFromPoint(AtAngleDeg(355), 4), 0);
	TestTrue(TEXT("center"), DIRingLayout::IsCenter(FVector2D(10, -20)));
	TestFalse(TEXT("ring is not center"), DIRingLayout::IsCenter(AtAngleDeg(10)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDILayoutTapTest, "DoubleIt.Layout.TapRouting", DILayoutTestFlags)
bool FDILayoutTapTest::RunTest(const FString&)
{
	FDIGame Game(EDIRuleSet::Doubling, 1);
	Game.SetRingForTest({ FDIPiece::Number(2), FDIPiece::Number(2), FDIPiece::Number(8) });

	Game.SetCenterForTest(FDIPiece::Plus());
	TestFalse(TEXT("center tap does nothing for a fresh plus"), DIRingLayout::ApplyTap(Game, FVector2D::ZeroVector));
	TestTrue(TEXT("tap between the 2s fuses"), DIRingLayout::ApplyTap(Game, AtAngleDeg(60)));
	TestEqual(TEXT("fused"), Game.GetRing().Num(), 2);

	Game.SetCenterForTest(FDIPiece::Minus());
	TestTrue(TEXT("minus absorbs nearest tile"), DIRingLayout::ApplyTap(Game, AtAngleDeg(5)));
	TestTrue(TEXT("center tap converts absorbed piece"), DIRingLayout::ApplyTap(Game, FVector2D::ZeroVector));
	TestTrue(TEXT("center is plus"), Game.GetCenter().Kind == EDIPieceKind::Plus);
	return true;
}

#endif
