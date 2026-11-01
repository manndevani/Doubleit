#include "DIRingLayout.h"
#include "DIGame.h"

FVector2D DIRingLayout::TilePosition(int32 Index, int32 Count)
{
	const float Angle = Count > 0 ? UE_TWO_PI * Index / Count : 0.f;
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

int32 DIRingLayout::SlotFromPoint(const FVector2D& P, int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}
	const float Step = UE_TWO_PI / Count;
	return FMath::Min(FMath::FloorToInt(AngleOf(P) / Step) + 1, Count);
}

int32 DIRingLayout::TileFromPoint(const FVector2D& P, int32 Count)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	const float Step = UE_TWO_PI / Count;
	return FMath::RoundToInt(AngleOf(P) / Step) % Count;
}

bool DIRingLayout::IsCenter(const FVector2D& P)
{
	return P.Size() <= CenterHitRadius;
}

bool DIRingLayout::ApplyTap(FDIGame& Game, const FVector2D& P)
{
	const int32 Count = Game.GetRing().Num();
	if (Game.GetCenter().Kind == EDIPieceKind::Minus)
	{
		return !IsCenter(P) && Game.AbsorbAt(TileFromPoint(P, Count));
	}
	if (IsCenter(P))
	{
		return Game.ConvertCenterToPlus();
	}
	return Game.PlaceAt(SlotFromPoint(P, Count));
}
