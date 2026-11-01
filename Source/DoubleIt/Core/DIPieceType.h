#pragma once

#include "DITypes.h"

struct FDIModeConfig;

/**
 * What a kind of piece does (Type Object). FDIGame, the spawner, the layout and the presenters ask the type instead of
 * checking for a specific kind. A new special piece = a new EDIPieceKind value (also its glyph id in DI_Tile.hlsl),
 * a type class here, a DIPieces::Register call and a spawn row in FDISpawnTuning. Spec: docs/GAME_DESIGN.md#pieces
 */
class DOUBLEIT_API IDIPieceType
{
public:
	virtual ~IDIPieceType() = default;

	/** Stable name: INI keys (`<Name>Chance`), sound cues, logs. */
	virtual FName Name() const = 0;

	/** Fuses equal numbers on both sides into one (the plus). */
	virtual bool IsCatalyst() const { return false; }

	/** As the centre piece, a tap takes a ring piece into the centre instead of placing (the minus). */
	virtual bool LiftsFromRing() const { return false; }

	/** A piece lifted by this one may be turned into this kind instead (minus -> plus). None = not offered. */
	virtual TOptional<EDIPieceKind> LiftedConvertsTo() const { return {}; }

	/** Placing inserts the piece into the ring; false = it only applies OnPlaced (the breeze). */
	virtual bool InsertsOnPlace() const { return true; }

	/** Effect on the ring when placed (after inserting, if it inserts). */
	virtual void OnPlaced(TArray<FDIPiece>& Ring, const FDIModeConfig& Config) const {}

	/** Sound cue when this piece lands (see UDIGameSettings::Cues). */
	virtual FName LandCue() const { return TEXT("Place"); }
};

namespace DIPieces
{
	/** The type of Kind. Number, Plus, Minus and Breeze are built in. */
	DOUBLEIT_API const IDIPieceType& Get(EDIPieceKind Kind);

	/** Adds or replaces the type for Kind. */
	DOUBLEIT_API void Register(EDIPieceKind Kind, TSharedRef<const IDIPieceType> Type);

	inline const IDIPieceType& Of(const FDIPiece& Piece) { return Get(Piece.Kind); }
}
