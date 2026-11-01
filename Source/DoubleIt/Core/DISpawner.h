#pragma once

#include "DITypes.h"
#include "Math/RandomStream.h"

class IDIRuleSet;

/** One special piece in the spawn table. Rows are rolled in order against one shared roll (cumulative chance). */
struct FDISpawnRow
{
	EDIPieceKind Kind = EDIPieceKind::Plus;
	float Chance = 0.f;
	int32 FromMove = 0;        // not before this many moves
	int32 MinRing = 0;         // not while the ring holds fewer pieces
	int32 GuaranteeWindow = 0; // forced if none in the last N spawns (0 = never forced)
	bool bNoRepeat = false;    // never twice in a row
};

/** Spawn tuning: number growth + the special-piece table. Spec: docs/GAME_DESIGN.md#spawning */
struct FDISpawnTuning
{
	int32 MovesPerRangeStep = 40; // numbers climb one level every N moves
	TArray<FDISpawnRow> Specials = {
		{ EDIPieceKind::Plus, 0.17f, 0, 0, 5, false },
		{ EDIPieceKind::Minus, 0.05f, 10, 0, 0, true },
	};

	FDISpawnRow* Find(EDIPieceKind Kind) { return Specials.FindByPredicate([Kind](const FDISpawnRow& R) { return R.Kind == Kind; }); }
	const FDISpawnRow* Find(EDIPieceKind Kind) const { return Specials.FindByPredicate([Kind](const FDISpawnRow& R) { return R.Kind == Kind; }); }
	float ChanceOf(EDIPieceKind Kind) const { const FDISpawnRow* R = Find(Kind); return R ? R->Chance : 0.f; }
};

class DOUBLEIT_API FDISpawner
{
public:
	/** Rules decides which numbers spawn (see DIMode.h); kinds and chances come from the tuning. */
	FDISpawner(TSharedRef<const IDIRuleSet> InRules, int32 Seed, const FDISpawnTuning& InTuning = FDISpawnTuning());

	FDIPiece Next(int32 Moves, int32 RingSize);

	/** Writes or reads the spawner (random state, guarantee counters, the run.s tuning) for a saved run. */
	void Serialize(FArchive& Ar);
	const FDISpawnTuning& GetTuning() const { return Tuning; }

private:
	FDIPiece NextNumber(int32 Moves);

	TSharedRef<const IDIRuleSet> Rules;
	FRandomStream Stream;
	FDISpawnTuning Tuning;
	TMap<EDIPieceKind, int32> SpawnsSince; // per special kind, for guarantee windows
	EDIPieceKind LastKind = EDIPieceKind::Number;
};
