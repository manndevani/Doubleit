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
	// One roll per spawn; each row is picked if forced by its guarantee, or allowed and under its cumulative chance.
	const float Roll = Stream.GetFraction();
	FDIPiece Result;
	bool bPicked = false;
	float Cumulative = 0.f;
	for (const FDISpawnRow& Row : Tuning.Specials)
	{
		Cumulative += Row.Chance;
		const bool bForced = Row.GuaranteeWindow > 0 && SpawnsSince.FindRef(Row.Kind) >= Row.GuaranteeWindow;
		const bool bAllowed = Moves >= Row.FromMove && RingSize >= Row.MinRing && !(Row.bNoRepeat && LastKind == Row.Kind);
		if (bForced || (bAllowed && Roll < Cumulative))
		{
			Result = FDIPiece{ Row.Kind, 0, 0 };
			bPicked = true;
			break;
		}
	}
	if (!bPicked)
	{
		Result = NextNumber(Moves);
	}

	for (const FDISpawnRow& Row : Tuning.Specials)
	{
		int32& Since = SpawnsSince.FindOrAdd(Row.Kind);
		Since = Row.Kind == Result.Kind ? 0 : Since + 1;
	}
	LastKind = Result.Kind;
	return Result;
}

FDIPiece FDISpawner::NextNumber(int32 Moves)
{
	return FDIPiece::Number(Rules->SpawnNumber(Moves / FMath::Max(1, Tuning.MovesPerRangeStep), Stream));
}
