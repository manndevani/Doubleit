#pragma once

#include "CoreMinimal.h"

struct FDIModeConfig;

/**
 * The pieces a player owns (the breeze): how many of each, and how far each mode has paid out rewards. Pure data and
 * rules; UDIShop keeps it in the save and feeds it game events. Keys are piece type names (IDIPieceType::Name).
 * Spec: docs/GAME_DESIGN.md#breeze
 */
struct DOUBLEIT_API FDIWallet
{
	TMap<FName, int32> Owned;
	/** Per mode key: the highest number already rewarded, so each threshold pays once, ever. */
	TMap<FName, int64> RewardedUpTo;

	int32 Count(FName Piece) const { return Owned.FindRef(Piece); }
	void Add(FName Piece, int32 Amount);
	/** Uses one; false if none is left. */
	bool Spend(FName Piece);

	/**
	 * A fusion made Value in Mode. Pays Config.RewardPiece once the first time ever a number at or above
	 * Config.RewardFrom is made, and once again for each new highest number after that. True if it paid.
	 */
	bool Reached(FName Mode, int64 Value, const FDIModeConfig& Config);
};
