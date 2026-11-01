#include "DIRingLayout.h"
#include "DIGame.h"

namespace
{
	/** Angle of P measured from the ring's own zero (tile 0), in [0, 2pi). */
	float LocalAngle(const FVector2D& P, float Offset)
	{
		return FMath::Fmod(DIRingLayout::AngleOf(P) - Offset + 4.f * UE_TWO_PI, UE_TWO_PI);
	}
}

float DIRingLayout::TileAngle(int32 Index, int32 Count, float Offset)
{
	return Offset + (Count > 0 ? UE_TWO_PI * Index / Count : 0.f);
}

FVector2D DIRingLayout::TilePosition(int32 Index, int32 Count, float Offset)
{
	const float Angle = TileAngle(Index, Count, Offset);
	return FVector2D(RingRadius * FMath::Sin(Angle), RingRadius * FMath::Cos(Angle));
}

float DIRingLayout::AngleOf(const FVector2D& P)
{
	float Angle = FMath::Atan2(P.X, P.Y);
	if (Angle < 0.f)
	{
		Angle += UE_TWO_PI;
	}
	return Angle;
}

int32 DIRingLayout::SlotFromPoint(const FVector2D& P, int32 Count, float Offset)
{
	if (Count <= 0)
	{
		return 0;
	}
	const float Step = UE_TWO_PI / Count;
	return FMath::Min(FMath::FloorToInt(LocalAngle(P, Offset) / Step) + 1, Count);
}

int32 DIRingLayout::TileFromPoint(const FVector2D& P, int32 Count, float Offset)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	const float Step = UE_TWO_PI / Count;
	return FMath::RoundToInt(LocalAngle(P, Offset) / Step) % Count;
}

bool DIRingLayout::IsCenter(const FVector2D& P)
{
	return P.Size() <= CenterHitRadius;
}

float DIRingLayout::Reanchor(const TArray<FDIPiece>& Before, float BeforeOffset, const TArray<FDIPiece>& After)
{
	// Circular mean of how far each surviving piece would move: choosing the offset that cancels it means a gap
	// opens or closes evenly on both sides and a fusion collapses onto the plus, instead of the ring turning one way.
	float SumSin = 0.f;
	float SumCos = 0.f;
	for (int32 a = 0; a < After.Num(); ++a)
	{
		const int32 b = Before.IndexOfByPredicate([&](const FDIPiece& P) { return P.Id == After[a].Id; });
		if (b != INDEX_NONE)
		{
			const float Delta = TileAngle(b, Before.Num(), BeforeOffset) - TileAngle(a, After.Num(), 0.f);
			SumSin += FMath::Sin(Delta);
			SumCos += FMath::Cos(Delta);
		}
	}
	if (FMath::IsNearlyZero(SumSin) && FMath::IsNearlyZero(SumCos))
	{
		return BeforeOffset;
	}
	return FMath::Atan2(SumSin, SumCos);
}

bool DIRingLayout::ApplyTap(FDIGame& Game, const FVector2D& P, float Offset)
{
	const int32 Count = Game.GetRing().Num();
	if (Game.GetCenter().Kind == EDIPieceKind::Minus)
	{
		return !IsCenter(P) && Game.AbsorbAt(TileFromPoint(P, Count, Offset));
	}
	if (IsCenter(P))
	{
		return Game.ConvertCenterToPlus();
	}
	return Game.PlaceAt(SlotFromPoint(P, Count, Offset));
}
