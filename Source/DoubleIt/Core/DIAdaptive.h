#pragma once

#include "DITypes.h"
#include "DISpawner.h"

/** What one finished run tells us about the player. Filled by the game mode, consumed by DIAdaptive::Learn. */
struct FDIRunSummary
{
	int32 Moves = 0;
	int32 BestChain = 0;
	int32 PlusesPlaced = 0;
	int32 Fusions = 0;          // first-step fusions (one per plus that fired)
	float AvgThinkSeconds = 0.f;
	int64 Highest = 0;
};

/**
 * Long-term picture of how a player plays one mode (each mode keeps its own). Smoothed so one lucky or unlucky
 * run barely moves it. Saved on the device; see docs/GAME_DESIGN.md#adaptive-pace.
 */
struct FDIPlayerProfile
{
	int32 Runs = 0;
	float AvgMoves = 0.f;
	float AvgBestChain = 0.f;
	float PlusYield = 0.f;       // fusions per plus placed: how well pluses are used
	float AvgThinkSeconds = 0.f; // recorded for future modes; not used for difficulty
	int64 HighestEver = 0;
};

namespace DIAdaptive
{
	/** Weight of the newest run in the running averages. */
	constexpr float LearnRate = 0.3f;

	DOUBLEIT_API void Learn(FDIPlayerProfile& Profile, const FDIRunSummary& Run);

	/** 0 = struggling, 0.5 = typical (or not enough data yet), 1 = strong. Confidence grows with runs played. */
	DOUBLEIT_API float Skill(const FDIPlayerProfile& Profile);

	/**
	 * Gentle, bounded spawn tuning for this player: struggling players get more pluses and slower number growth,
	 * strong players a slightly faster climb. Never outside plus chance 14-22% or growth step 34-50 moves.
	 */
	DOUBLEIT_API FDISpawnTuning TuningFor(const FDIPlayerProfile& Profile, const FDISpawnTuning& Base = FDISpawnTuning());
}
