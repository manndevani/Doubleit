#pragma once

#include "CoreMinimal.h"

enum class EDIPieceKind : uint8
{
	Number,
	Plus,
	Minus,
	Breeze
};

struct FDIPiece
{
	EDIPieceKind Kind = EDIPieceKind::Number;
	int64 Value = 0; // only meaningful for Number
	uint32 Id = 0;   // assigned by FDIGame; lets the renderer follow a piece across moves (not part of ==)

	static FDIPiece Number(int64 InValue) { return FDIPiece{ EDIPieceKind::Number, InValue, 0 }; }
	static FDIPiece Plus() { return FDIPiece{ EDIPieceKind::Plus, 0, 0 }; }
	static FDIPiece Minus() { return FDIPiece{ EDIPieceKind::Minus, 0, 0 }; }
	static FDIPiece Breeze() { return FDIPiece{ EDIPieceKind::Breeze, 0, 0 }; }

	bool IsNumber() const { return Kind == EDIPieceKind::Number; }
	bool operator==(const FDIPiece& Other) const { return Kind == Other.Kind && Value == Other.Value; }
};

/** One fusion step, emitted so the renderer can animate it. */
struct FDIFusionEvent
{
	int32 ChainDepth = 0;
	int64 ResultValue = 0;
	uint32 ResultId = 0;
	uint32 ConsumedIds[3] = { 0, 0, 0 }; // left, right, previous centre (the plus on the first step)
	TArray<FDIPiece> RingAfter;           // the ring right after this step, so chains can be shown one collision at a time
};
