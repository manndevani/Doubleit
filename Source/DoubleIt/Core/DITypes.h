#pragma once

#include "CoreMinimal.h"

enum class EDIPieceKind : uint8
{
	Number,
	Plus,
	Minus,
	Breeze
};

enum class EDIRuleSet : uint8
{
	Doubling,
	Addition
};

struct FDIPiece
{
	EDIPieceKind Kind = EDIPieceKind::Number;
	int64 Value = 0; // only meaningful for Number

	static FDIPiece Number(int64 InValue) { return FDIPiece{ EDIPieceKind::Number, InValue }; }
	static FDIPiece Plus() { return FDIPiece{ EDIPieceKind::Plus, 0 }; }
	static FDIPiece Minus() { return FDIPiece{ EDIPieceKind::Minus, 0 }; }
	static FDIPiece Breeze() { return FDIPiece{ EDIPieceKind::Breeze, 0 }; }

	bool IsNumber() const { return Kind == EDIPieceKind::Number; }
	bool operator==(const FDIPiece& Other) const { return Kind == Other.Kind && Value == Other.Value; }
};

/** One fusion step, emitted so the renderer can animate it. */
struct FDIFusionEvent
{
	int32 ChainDepth = 0;
	int64 ResultValue = 0;
};
