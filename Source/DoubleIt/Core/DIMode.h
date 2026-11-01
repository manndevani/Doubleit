#pragma once

#include "DITypes.h"
#include "DIAdaptive.h"
#include "DISpawner.h"
#include "Math/RandomStream.h"

/**
 * Game modes. Spec: docs/GAME_DESIGN.md#modes
 *
 * A mode = a rule set (Strategy: the maths that differs between modes) + a config (data: every tunable number).
 * FDIGame, FDISpawner and the renderer only ask the mode; nothing switches on a mode name. To add a mode, write an
 * IDIRuleSet, then DIModes::Register it with a key. Configs can be overridden per mode in Config/DefaultGame.ini.
 */
class DOUBLEIT_API IDIRuleSet
{
public:
	virtual ~IDIRuleSet() = default;

	/** A plus between two equal numbers Outer. */
	virtual int64 FirstFusion(int64 Outer) const = 0;

	/** An equal outer pair Outer wrapping an already-fused Center. */
	virtual int64 ChainFusion(int64 Center, int64 Outer) const = 0;

	/** A spawned number at growth level Level (moves / MovesPerRangeStep). */
	virtual int64 SpawnNumber(int32 Level, FRandomStream& Stream) const = 0;

	/** Palette index for a number (the renderer cycles the palette). */
	virtual int32 ColourRank(int64 Value) const = 0;

	/** Points for a fusion result at chain depth Depth (1 = first step). */
	virtual int64 FusionScore(int64 Result, int32 Depth) const { return Result * Depth; }
};

/** Every tunable number of one mode. Defaults are Classic's. */
struct FDIModeConfig
{
	int32 StartRingPieces = 6;
	int32 MaxRingPieces = 18;      // game over when the ring holds more than this
	float BreezeClearFraction = 0.5f; // share of the ring a breeze removes, lowest numbers first
	int64 RevealFrom = 32;         // first-ever numbers from here up get the reveal chime
	FDISpawnTuning Spawn;
	FDIAdaptiveTuning Adaptive;
};

struct FDIMode
{
	FName Key;
	TSharedPtr<const IDIRuleSet> Rules;
	FDIModeConfig Config;

	bool IsValid() const { return Rules.IsValid(); }
};

/** Registry of modes by key. Classic and Addition are built in. */
namespace DIModes
{
	inline const TCHAR* Classic = TEXT("Classic");
	inline const TCHAR* Addition = TEXT("Addition");

	/** Adds a mode, or replaces the one with the same key. */
	DOUBLEIT_API void Register(const FDIMode& Mode);

	DOUBLEIT_API bool Contains(FName Key);

	/** The mode for Key, or Classic when Key is unknown. Returned by value: callers may tune their copy. */
	DOUBLEIT_API FDIMode Get(FName Key);

	/** Registered keys, in registration order. */
	DOUBLEIT_API TArray<FName> Keys();
}
