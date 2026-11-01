#pragma once

#include "CoreMinimal.h"

class FDIGame;

/** Where to tap: a point in ring space (DIRingLayout) and the size of what is there (a gap, a stone, the centre). */
struct FDITapTarget
{
	FVector2D Pos = FVector2D::ZeroVector;
	float Size = 0.f;
};

enum class EDITutorialResult : uint8
{
	Continue, // keep playing this step
	Advance,  // step done; call SetupStep for the next one (or finish)
	Retry     // wrong move; call SetupStep again
};

/** Scripted first-run tutorial. Spec: docs/GAME_DESIGN.md#tutorial */
class DOUBLEIT_API FDITutorial
{
public:
	static constexpr int32 NumSteps = 6;

	void Reset(int32 First = 0) { Step = FMath::Clamp(First, 0, NumSteps - 1); }
	int32 GetStep() const { return Step; }
	bool IsFinished() const { return Step >= NumSteps; }
	/** String-table key of this step's instruction (Tutorial.<step>), or empty when done. Text: Game/DIText.h. */
	FString GetInstructionKey() const;

	/** Puts the step's scripted board into Game. */
	void SetupStep(FDIGame& Game) const;

	/**
	 * What this step makes of a board after a move, without moving on: Advance (the lesson), Continue (fine, keep
	 * going) or Retry (not what this step teaches). The game mode tries each tap on a copy first and only plays it
	 * when this is not Retry, so a wrong move is never played and then taken back.
	 */
	EDITutorialResult Judge(const FDIGame& After) const;

	/**
	 * The tap that completes this step on Game (ring at Offset): every possible tap (centre, each gap, each stone) is
	 * tried on a copy and judged, so the hint can never point the wrong way. The centre is preferred, then gaps for a
	 * piece that is placed (stones for one that lifts); when several of one kind complete it, the middle one is shown.
	 */
	TOptional<FDITapTarget> FindTarget(const FDIGame& Game, float Offset) const;

	/** Call after every action that changed the board. Moves to the next step on Advance. */
	EDITutorialResult Evaluate(const FDIGame& Game);

private:
	int32 Step = 0;
};
