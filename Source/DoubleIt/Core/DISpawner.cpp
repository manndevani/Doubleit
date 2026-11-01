#include "DISpawner.h"

FDISpawner::FDISpawner(EDIRuleSet InRules, int32 Seed, const FDISpawnTuning& InTuning)
	: Rules(InRules)
	, Stream(Seed)
	, Tuning(InTuning)
{
}

FDIPiece FDISpawner::Next(int32 Moves, int32 RingSize)
{
	FDIPiece Result;
	const float Roll = Stream.GetFraction();

	if (SpawnsSincePlus >= Tuning.PlusGuaranteeWindow || Roll < Tuning.PlusChance)
	{
		Result = FDIPiece::Plus();
	}
	else if (Moves >= Tuning.MinusFromMove && !bLastWasMinus && Roll < Tuning.PlusChance + Tuning.MinusChance)
	{
		Result = FDIPiece::Minus();
	}
	else if (Moves >= Tuning.BreezeFromMove && RingSize >= Tuning.BreezeMinRing
		&& Roll < Tuning.PlusChance + Tuning.MinusChance + Tuning.BreezeChance)
	{
		Result = FDIPiece::Breeze();
	}
	else
	{
		Result = NextNumber(Moves);
	}

	SpawnsSincePlus = Result.Kind == EDIPieceKind::Plus ? 0 : SpawnsSincePlus + 1;
	bLastWasMinus = Result.Kind == EDIPieceKind::Minus;
	return Result;
}

FDIPiece FDISpawner::NextNumber(int32 Moves)
{
	const int32 Base = Moves / FMath::Max(1, Tuning.MovesPerRangeStep);
	if (Rules == EDIRuleSet::Doubling)
	{
		const int32 Offset = Stream.RandRange(0, 2);
		return FDIPiece::Number(int64(1) << FMath::Min(Base + Offset, 62));
	}
	// Addition merges leave the spawn range quickly, so spawn only two adjacent values to keep pairs common.
	return FDIPiece::Number(1 + Base + Stream.RandRange(0, 1));
}
