#include "DISpawner.h"
#include "DIMode.h"

FDISpawner::FDISpawner(TSharedRef<const IDIRuleSet> InRules, int32 Seed, const FDISpawnTuning& InTuning)
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
	return FDIPiece::Number(Rules->SpawnNumber(Moves / FMath::Max(1, Tuning.MovesPerRangeStep), Stream));
}
