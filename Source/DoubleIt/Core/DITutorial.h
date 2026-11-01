#pragma once

#include "CoreMinimal.h"

class FDIGame;

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

	/** Call after every action that changed the board. Moves to the next step on Advance. */
	EDITutorialResult Evaluate(const FDIGame& Game);

private:
	int32 Step = 0;
};
