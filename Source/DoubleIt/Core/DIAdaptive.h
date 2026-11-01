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

/** How the adaptive pace reads a player and how far it may move the spawner. Part of each mode's config. */
struct FDIAdaptiveTuning
{
	float LearnRate = 0.3f;         // weight of the newest run in the running averages
	float PriorRuns = 2.f;          // confidence = runs / (runs + PriorRuns): one run is a hint, five are a pattern
	int32 MinAbandonedMoves = 15;  // an abandoned run teaches us something only from here
	float MaxThinkSeconds = 30.f;   // a phone set down mid-run should not read as slow thinking
	float SurvivalLow = 30.f;       // average moves that read as skill 0 .. 1
	float SurvivalHigh = 120.f;
	float ChainLow = 1.f;           // average best chain that reads as skill 0 .. 1
	float ChainHigh = 5.f;
	float SurvivalWeight = 0.6f;
	float ChainWeight = 0.25f;
	float YieldWeight = 0.15f;
	float HelpPlusChance = 0.05f;   // at full help (help is stronger than push: early losses drive quitting)
	int32 HelpGrowthMoves = 10;
	float HelpWindowFrom = 0.5f;    // help above this also shortens the plus guarantee window by one
	float PushPlusChance = 0.03f;   // at full push
	int32 PushGrowthMoves = 6;
	float MinPlusChance = 0.14f;
	float MaxPlusChance = 0.22f;
	int32 MinGrowthMoves = 34;
	int32 MaxGrowthMoves = 50;
};

namespace DIAdaptive
{
	DOUBLEIT_API void Learn(FDIPlayerProfile& Profile, const FDIRunSummary& Run, const FDIAdaptiveTuning& A = FDIAdaptiveTuning());

	/** 0 = struggling, 0.5 = typical (or not enough data yet), 1 = strong. Confidence grows with runs played. */
	DOUBLEIT_API float Skill(const FDIPlayerProfile& Profile, const FDIAdaptiveTuning& A = FDIAdaptiveTuning());

	/**
	 * Gentle, bounded spawn tuning for this player: struggling players get more pluses and slower number growth,
	 * strong players a slightly faster climb. Never outside the bounds in A.
	 */
	DOUBLEIT_API FDISpawnTuning TuningFor(const FDIPlayerProfile& Profile, const FDISpawnTuning& Base = FDISpawnTuning(),
		const FDIAdaptiveTuning& A = FDIAdaptiveTuning());
}
