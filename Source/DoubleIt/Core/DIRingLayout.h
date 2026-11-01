#pragma once

#include "DITypes.h"

class FDIGame;

/**
 * Ring geometry in a 2D screen-aligned space: X right, Y up, origin at the ring centre.
 * Tile 0 sits at the offset angle (0 = top); indices go clockwise. A slot k means "insert before Ring[k]".
 */
namespace DIRingLayout
{
	constexpr float RingRadius = 300.f;
	constexpr float TileSize = 70.f;
	constexpr float CenterSize = 100.f;
	constexpr float CenterHitRadius = 80.f;

	/** Offset rotates the whole ring (radians, clockwise): the ring has no fixed top, see Reanchor. */
	DOUBLEIT_API float TileAngle(int32 Index, int32 Count, float Offset = 0.f);
	DOUBLEIT_API FVector2D TilePosition(int32 Index, int32 Count, float Offset = 0.f);

	/** Clockwise angle from the top, in [0, 2pi). */
	DOUBLEIT_API float AngleOf(const FVector2D& P);

	DOUBLEIT_API int32 SlotFromPoint(const FVector2D& P, int32 Count, float Offset = 0.f);
	DOUBLEIT_API int32 TileFromPoint(const FVector2D& P, int32 Count, float Offset = 0.f);
	DOUBLEIT_API bool IsCenter(const FVector2D& P);

	/** Routes a tap to the right game action. Returns true if the board changed. */
	DOUBLEIT_API bool ApplyTap(FDIGame& Game, const FVector2D& P, float Offset = 0.f);

	/** Offset for After that keeps pieces (matched by id) as still as possible relative to Before. */
	DOUBLEIT_API float Reanchor(const TArray<FDIPiece>& Before, float BeforeOffset, const TArray<FDIPiece>& After);
}
