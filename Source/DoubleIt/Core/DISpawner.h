#pragma once

#include "DITypes.h"
#include "Math/RandomStream.h"

class IDIRuleSet;

/** Spawn tuning. Spec: docs/GAME_DESIGN.md#spawning */
struct FDISpawnTuning
{
	int32 MovesPerRangeStep = 40;
	int32 PlusGuaranteeWindow = 5;   // a Plus is forced if none in the last N spawns
	float PlusChance = 0.17f;
	float MinusChance = 0.05f;
	int32 MinusFromMove = 10;
	float BreezeChance = 0.015f;
	int32 BreezeFromMove = 30;
	int32 BreezeMinRing = 12;
};

class DOUBLEIT_API FDISpawner
{
public:
	/** Rules decides which numbers spawn (see DIMode.h); kinds and chances come from the tuning. */
	FDISpawner(TSharedRef<const IDIRuleSet> InRules, int32 Seed, const FDISpawnTuning& InTuning = FDISpawnTuning());

	FDIPiece Next(int32 Moves, int32 RingSize);

private:
	FDIPiece NextNumber(int32 Moves);

	TSharedRef<const IDIRuleSet> Rules;
	FRandomStream Stream;
	FDISpawnTuning Tuning;
	int32 SpawnsSincePlus = 0;
	bool bLastWasMinus = false;
};
