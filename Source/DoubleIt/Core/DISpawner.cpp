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

void FDISpawner::Serialize(FArchive& Ar)
{
	int32 Seed = Stream.GetCurrentSeed();
	Ar << Seed;
	if (Ar.IsLoading())
	{
		Stream.Initialize(Seed);
	}

	Ar << Tuning.MovesPerRangeStep;
	int32 Rows = Tuning.Specials.Num();
	Ar << Rows;
	if (Ar.IsLoading())
	{
		Tuning.Specials.SetNum(FMath::Clamp(Rows, 0, 64));
	}
	for (FDISpawnRow& Row : Tuning.Specials)
	{
		uint8 Kind = uint8(Row.Kind);
		Ar << Kind << Row.Chance << Row.FromMove << Row.MinRing << Row.GuaranteeWindow << Row.bNoRepeat;
		Row.Kind = EDIPieceKind(Kind);
	}

	uint8 Last = uint8(LastKind);
	Ar << Last;
	LastKind = EDIPieceKind(Last);
	TArray<TPair<uint8, int32>> Since;
	for (const TPair<EDIPieceKind, int32>& Pair : SpawnsSince)
	{
		Since.Emplace(uint8(Pair.Key), Pair.Value);
	}
	int32 Count = Since.Num();
	Ar << Count;
	if (Ar.IsLoading())
	{
		Since.SetNum(FMath::Clamp(Count, 0, 64));
	}
	for (TPair<uint8, int32>& Pair : Since)
	{
		Ar << Pair.Key << Pair.Value;
	}
	if (Ar.IsLoading())
	{
		SpawnsSince.Reset();
		for (const TPair<uint8, int32>& Pair : Since)
		{
			SpawnsSince.Add(EDIPieceKind(Pair.Key), Pair.Value);
		}
	}
}
