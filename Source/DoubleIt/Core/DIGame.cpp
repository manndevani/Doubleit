#include "DIGame.h"
#include "DIRules.h"

FDIGame::FDIGame(EDIRuleSet InRules, int32 Seed, const FDISpawnTuning& Tuning)
	: Rules(InRules)
	, Spawner(InRules, Seed, Tuning)
{
	for (int32 i = 0; i < 6; ++i)
	{
		FDIPiece Piece = Spawner.Next(0, Ring.Num());
		while (!Piece.IsNumber())
		{
			Piece = Spawner.Next(0, Ring.Num());
		}
		Ring.Add(Piece);
	}
	Center = Spawner.Next(0, Ring.Num());
}

bool FDIGame::PlaceAt(int32 Slot)
{
	if (bGameOver || Center.Kind == EDIPieceKind::Minus || Slot < 0 || Slot > Ring.Num())
	{
		return false;
	}

	LastEvents.Reset();
	if (Center.Kind == EDIPieceKind::Breeze)
	{
		ApplyBreeze();
	}
	else
	{
		Ring.Insert(Center, Slot);
		if (Center.IsNumber())
		{
			HighestValue = FMath::Max(HighestValue, Center.Value);
		}
	}

	ResolveAll();
	FinishMove();
	return true;
}

bool FDIGame::AbsorbAt(int32 Index)
{
	if (bGameOver || Center.Kind != EDIPieceKind::Minus || !Ring.IsValidIndex(Index))
	{
		return false;
	}
	Center = Ring[Index];
	Ring.RemoveAt(Index);
	bCenterAbsorbed = true;
	return true;
}

bool FDIGame::ConvertCenterToPlus()
{
	if (bGameOver || !bCenterAbsorbed)
	{
		return false;
	}
	Center = FDIPiece::Plus();
	bCenterAbsorbed = false;
	return true;
}

void FDIGame::ResolveAll()
{
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (int32 i = 0; i < Ring.Num(); ++i)
		{
			if (Ring[i].Kind == EDIPieceKind::Plus && TryFuseAt(i))
			{
				bChanged = true;
				break;
			}
		}
	}
}

bool FDIGame::TryFuseAt(int32 PlusIndex)
{
	int32 CenterIndex = PlusIndex;
	int32 Depth = 0;

	while (Ring.Num() >= 3)
	{
		const int32 Num = Ring.Num();
		const FDIPiece& L = Ring[(CenterIndex - 1 + Num) % Num];
		const FDIPiece& R = Ring[(CenterIndex + 1) % Num];
		if (!L.IsNumber() || !R.IsNumber() || L.Value != R.Value)
		{
			break;
		}

		const int64 Outer = L.Value;
		const int64 NewValue = Depth == 0
			? DIRules::FirstFusion(Rules, Outer)
			: DIRules::ChainFusion(Rules, Ring[CenterIndex].Value, Outer);

		RemoveNeighbours(CenterIndex);
		Ring[CenterIndex] = FDIPiece::Number(NewValue);
		++Depth;

		Score += NewValue * Depth;
		HighestValue = FMath::Max(HighestValue, NewValue);
		LastEvents.Add(FDIFusionEvent{ Depth, NewValue });
	}
	return Depth > 0;
}

void FDIGame::RemoveNeighbours(int32& CenterIndex)
{
	const int32 Num = Ring.Num();
	const int32 L = (CenterIndex - 1 + Num) % Num;
	const int32 R = (CenterIndex + 1) % Num;
	const int32 Hi = FMath::Max(L, R);
	const int32 Lo = FMath::Min(L, R);

	Ring.RemoveAt(Hi);
	if (Hi < CenterIndex)
	{
		--CenterIndex;
	}
	Ring.RemoveAt(Lo);
	if (Lo < CenterIndex)
	{
		--CenterIndex;
	}
}

void FDIGame::ApplyBreeze()
{
	int32 ToRemove = Ring.Num() / 2;
	while (ToRemove-- > 0)
	{
		int32 Lowest = INDEX_NONE;
		for (int32 i = 0; i < Ring.Num(); ++i)
		{
			if (Ring[i].IsNumber() && (Lowest == INDEX_NONE || Ring[i].Value < Ring[Lowest].Value))
			{
				Lowest = i;
			}
		}
		if (Lowest == INDEX_NONE)
		{
			break;
		}
		Ring.RemoveAt(Lowest);
	}
}

void FDIGame::FinishMove()
{
	++Moves;
	bCenterAbsorbed = false;
	if (Ring.Num() > MaxRingPieces)
	{
		bGameOver = true;
		return;
	}
	Center = Spawner.Next(Moves, Ring.Num());
}
