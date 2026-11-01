#include "DITutorial.h"
#include "DIGame.h"

FString FDITutorial::GetInstructionKey() const
{
	return Step >= 0 && Step < NumSteps ? FString::Printf(TEXT("Tutorial.%d"), Step) : FString();
}

void FDITutorial::SetupStep(FDIGame& Game) const
{
	switch (Step)
	{
	case 0: Game.SetBoard(FDIPiece::Numbers({ 2, 4, 8, 4 }), FDIPiece::Number(2)); break;
	case 1: Game.SetBoard(FDIPiece::Numbers({ 2, 2, 8, 4 }), FDIPiece::Plus()); break;
	case 2: Game.SetBoard(FDIPiece::Numbers({ 8, 2, 2, 8, 1, 4 }), FDIPiece::Plus()); break;
	case 3: Game.SetBoard(FDIPiece::Numbers({ 2, 4, 16, 4, 8 }), FDIPiece::Minus()); break;
	case 4:
		Game.SetBoard(FDIPiece::Numbers({ 4, 2, 16, 2, 8 }), FDIPiece::Minus());
		Game.AbsorbAt(2);
		break;
	case 5: Game.SetBoard(FDIPiece::Numbers({ 1, 8, 2, 16, 1, 4, 2, 32, 1, 8, 2, 4, 1, 16, 2, 8 }), FDIPiece::Breeze()); break;
	default: break;
	}
}

EDITutorialResult FDITutorial::Judge(const FDIGame& Game) const
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
		return EDITutorialResult::Advance;
	}
	return bWrong ? EDITutorialResult::Retry : EDITutorialResult::Continue;
}

EDITutorialResult FDITutorial::Evaluate(const FDIGame& Game)
{
	const EDITutorialResult Result = Judge(Game);
	if (Result == EDITutorialResult::Advance)
	{
		++Step;
	}
	return Result;
}
