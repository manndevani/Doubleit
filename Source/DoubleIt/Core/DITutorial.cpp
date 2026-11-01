#include "DITutorial.h"
#include "DIGame.h"
#include "DIPieceType.h"
#include "DIRingLayout.h"

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

TOptional<FDITapTarget> FDITutorial::FindTarget(const FDIGame& Game, float Offset) const
{
	using namespace DIRingLayout;
	const int32 Num = Game.GetRing().Num();
	TArray<FDITapTarget> Gaps;
	for (int32 Slot = 0; Slot < FMath::Max(Num, 1); ++Slot)
	{
		// The middle of the gap before Ring[Slot] (the top of an empty ring).
		const float Angle = Num > 0 ? TileAngle(Slot, Num, Offset) - UE_PI / Num : 0.f;
		Gaps.Add({ RingRadius * FVector2D(FMath::Sin(Angle), FMath::Cos(Angle)), TileSize });
	}
	TArray<FDITapTarget> Stones;
	for (int32 i = 0; i < Num; ++i)
	{
		Stones.Add({ TilePosition(i, Num, Offset), TileSize });
	}

	// Groups in the order a player would naturally tap: the centre first, then what the centre piece acts on. A tap
	// near a stone and a tap in the next gap often do the same thing (a plus lands beside the stone; a minus lifts the
	// nearest stone), so the hint shows the gap for a piece that is placed and the stone for one that lifts.
	const bool bLifts = DIPieces::Of(Game.GetCenter()).LiftsFromRing();
	const TArray<TArray<FDITapTarget>> Groups = {
		{ { FVector2D::ZeroVector, CenterSize } },
		bLifts ? Stones : Gaps,
		bLifts ? Gaps : Stones,
	};

	for (const TArray<FDITapTarget>& Group : Groups)
	{
		TArray<FDITapTarget> Completing;
		for (const FDITapTarget& Candidate : Group)
		{
			FDIGame Trial = Game;
			if (ApplyTap(Trial, Candidate.Pos, Offset) && Judge(Trial) == EDITutorialResult::Advance)
			{
				Completing.Add(Candidate);
			}
		}
		if (Completing.Num() > 0)
		{
			return Completing[Completing.Num() / 2];
		}
	}
	return {};
}
