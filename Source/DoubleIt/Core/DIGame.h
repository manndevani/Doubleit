#pragma once

#include "DITypes.h"
#include "DISpawner.h"

/**
 * Complete game state for one run. Pure C++, deterministic for a given seed.
 * Ring is circular: Ring[0] and Ring.Last() are neighbours.
 * Spec: docs/GAME_DESIGN.md
 */
class DOUBLEIT_API FDIGame
{
public:
	static constexpr int32 MaxRingPieces = 18;

	FDIGame(EDIRuleSet InRules, int32 Seed, const FDISpawnTuning& Tuning = FDISpawnTuning());

	/** Insert the center piece before Ring[Slot] (Slot == Ring.Num() appends). Not valid for Minus. */
	bool PlaceAt(int32 Slot);

	/** Center is Minus: remove Ring[Index] and make it the new center. */
	bool AbsorbAt(int32 Index);

	/** Center came from a Minus: turn it into a Plus. */
	bool ConvertCenterToPlus();

	const TArray<FDIPiece>& GetRing() const { return Ring; }
	const FDIPiece& GetCenter() const { return Center; }
	EDIRuleSet GetRules() const { return Rules; }
	int64 GetScore() const { return Score; }
	int64 GetHighestValue() const { return HighestValue; }
	int32 GetMoves() const { return Moves; }
	bool IsGameOver() const { return bGameOver; }
	bool CanConvertCenter() const { return bCenterAbsorbed; }
	const TArray<FDIFusionEvent>& GetLastEvents() const { return LastEvents; }

	void SetRingForTest(const TArray<FDIPiece>& InRing) { Ring = InRing; }
	void SetCenterForTest(const FDIPiece& InCenter) { Center = InCenter; bCenterAbsorbed = false; }

private:
	void ResolveAll();
	bool TryFuseAt(int32 PlusIndex);
	void ApplyBreeze();
	void FinishMove();
	void RemoveNeighbours(int32& CenterIndex);

	EDIRuleSet Rules;
	FDISpawner Spawner;
	TArray<FDIPiece> Ring;
	FDIPiece Center;
	TArray<FDIFusionEvent> LastEvents;
	int64 Score = 0;
	int64 HighestValue = 0;
	int32 Moves = 0;
	bool bGameOver = false;
	bool bCenterAbsorbed = false;
};
