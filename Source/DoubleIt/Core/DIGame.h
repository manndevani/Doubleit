#pragma once

#include "DITypes.h"
#include "DIMode.h"
#include "DISpawner.h"

/**
 * Complete game state for one run. Pure C++, deterministic for a given seed.
 * Ring is circular: Ring[0] and Ring.Last() are neighbours.
 * Spec: docs/GAME_DESIGN.md
 */
class DOUBLEIT_API FDIGame
{
public:
	/** Mode: rules + config (DIModes::Get). Tune Mode.Config before passing it, e.g. for the adaptive pace. */
	FDIGame(const FDIMode& InMode, int32 Seed);

	/** Insert the center piece before Ring[Slot] (Slot == Ring.Num() appends). Not valid for Minus. */
	bool PlaceAt(int32 Slot);

	/** Center is Minus: remove Ring[Index] and make it the new center. */
	bool AbsorbAt(int32 Index);

	/** Center was lifted by a piece that allows it (minus): turn it into that piece's conversion (a plus). */
	bool ConvertCenterToPlus();

	/**
	 * Brings a piece the player owns (IDIPieceType::IsOwned, the breeze) into the centre. The current centre waits and
	 * comes back after the owned piece is used, or at once with ReturnHeld. False if one is already in, or game over.
	 */
	bool BringIn(EDIPieceKind Kind);

	/** Puts a brought-in piece back unused and restores the centre that waited. */
	bool ReturnHeld();

	/** A brought-in piece is in the centre (not yet used). */
	bool IsHolding() const { return Waiting.IsSet(); }

	/** Kind of the piece the last PlaceAt placed (Number before any move). */
	EDIPieceKind GetLastPlaced() const { return LastPlaced; }

	const TArray<FDIPiece>& GetRing() const { return Ring; }
	const FDIPiece& GetCenter() const { return Center; }
	const FDIMode& GetMode() const { return Mode; }
	int32 GetMaxRingPieces() const { return Mode.Config.MaxRingPieces; }
	int64 GetScore() const { return Score; }
	int64 GetHighestValue() const { return HighestValue; }
	int32 GetMoves() const { return Moves; }
	bool IsGameOver() const { return bGameOver; }
	bool CanConvertCenter() const { return bCenterAbsorbed; }
	const TArray<FDIFusionEvent>& GetLastEvents() const { return LastEvents; }
	/** Ring right after the last move placed its piece (or breeze cleared), before any fusion. */
	const TArray<FDIPiece>& GetPlacedRing() const { return PlacedRing; }

	/**
	 * Writes or reads the whole run (Ar.IsLoading()), so it survives the app closing. The rules come from this game.s
	 * mode; the spawn tuning and random state travel with the run, so a resumed run plays on exactly as it would have.
	 * Returns false (and leaves the game unusable) if the data is corrupt or from another format.
	 */
	bool SerializeRun(FArchive& Ar);
	static constexpr int32 RunFormat = 2; // 2: the centre waiting behind a brought-in piece

	/** Scripted board (tutorial, tests). Assigns fresh ids and clears transient state. */
	void SetBoard(const TArray<FDIPiece>& InRing, const FDIPiece& InCenter);

	void SetRingForTest(const TArray<FDIPiece>& InRing);
	void SetCenterForTest(const FDIPiece& InCenter) { Center = Tag(InCenter); bCenterAbsorbed = false; }

private:
	FDIPiece Tag(FDIPiece Piece) { Piece.Id = NextId++; return Piece; }
	void ResolveAll();
	bool TryFuseAt(int32 PlusIndex);
	void FinishMove();
	void RemoveNeighbours(int32& CenterIndex);

	FDIMode Mode;
	FDISpawner Spawner;
	TArray<FDIPiece> Ring;
	FDIPiece Center;
	TArray<FDIFusionEvent> LastEvents;
	TArray<FDIPiece> PlacedRing;
	int64 Score = 0;
	int64 HighestValue = 0;
	int32 Moves = 0;
	uint32 NextId = 1;
	bool bGameOver = false;
	bool bCenterAbsorbed = false;
	TOptional<EDIPieceKind> LiftConvertsTo; // what the lifting piece lets the lifted centre become

	/** The centre set aside by BringIn, with its lift state, until the owned piece is used or returned. */
	struct FWaiting
	{
		FDIPiece Center;
		bool bAbsorbed = false;
		TOptional<EDIPieceKind> ConvertsTo;
	};
	TOptional<FWaiting> Waiting;
	EDIPieceKind LastPlaced = EDIPieceKind::Number;
};
