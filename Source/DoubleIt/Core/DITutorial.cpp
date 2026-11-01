#include "DITutorial.h"
#include "DIGame.h"

namespace
{
	TArray<FDIPiece> Nums(std::initializer_list<int64> Values)
	{
		TArray<FDIPiece> Out;
		for (int64 V : Values)
		{
			Out.Add(FDIPiece::Number(V));
		}
		return Out;
	}
}

FString FDITutorial::GetInstruction() const
{
	switch (Step)
	{
	case 0: return TEXT("Tap a gap in the ring to place the piece in the middle.");
	case 1: return TEXT("A plus merges two equal neighbours into their double.");
	case 2: return TEXT("Symmetry around a plus starts a chain. Find it.");
	case 3: return TEXT("A minus lifts a piece out of the ring. Tap one.");
	case 4: return TEXT("Tap the centre to turn the lifted piece into a plus.");
	case 5: return TEXT("A full ring ends the run. A breeze clears the lowest half.");
	default: return FString();
	}
}

void FDITutorial::SetupStep(FDIGame& Game) const
{
	switch (Step)
	{
	case 0: Game.SetBoard(Nums({ 2, 4, 8, 4 }), FDIPiece::Number(2)); break;
	case 1: Game.SetBoard(Nums({ 2, 2, 8, 4 }), FDIPiece::Plus()); break;
	case 2: Game.SetBoard(Nums({ 8, 2, 2, 8, 1, 4 }), FDIPiece::Plus()); break;
	case 3: Game.SetBoard(Nums({ 2, 4, 16, 4, 8 }), FDIPiece::Minus()); break;
	case 4:
		Game.SetBoard(Nums({ 4, 2, 16, 2, 8 }), FDIPiece::Minus());
		Game.AbsorbAt(2);
		break;
	case 5: Game.SetBoard(Nums({ 1, 8, 2, 16, 1, 4, 2, 32, 1, 8, 2, 4, 1, 16, 2, 8 }), FDIPiece::Breeze()); break;
	default: break;
	}
}

EDITutorialResult FDITutorial::Evaluate(const FDIGame& Game)
{
	bool bDone = false;
	bool bWrong = false;
	switch (Step)
	{
	case 0: bDone = true; break;
	case 1: bDone = Game.GetLastEvents().Num() >= 1; bWrong = !bDone; break;
	case 2: bDone = Game.GetLastEvents().Num() >= 2; bWrong = !bDone; break;
	case 3: bDone = Game.CanConvertCenter(); break;
	case 4: bDone = Game.GetCenter().Kind == EDIPieceKind::Plus; bWrong = !bDone; break;
	case 5: bDone = true; break;
	default: break;
	}

	if (bDone)
	{
		++Step;
		return EDITutorialResult::Advance;
	}
	return bWrong ? EDITutorialResult::Retry : EDITutorialResult::Continue;
}
