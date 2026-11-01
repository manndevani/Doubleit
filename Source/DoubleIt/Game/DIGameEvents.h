#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/DITypes.h"
#include "DIGameEvents.generated.h"

DECLARE_MULTICAST_DELEGATE(FDIEvent);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDIPieceEvent, const FDIPiece& /*Piece*/, float /*Angle*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDIFusionStepEvent, const FDIFusionEvent& /*Step*/, float /*Angle*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FDIChainEvent, int32 /*Depth*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FDIFillEvent, float /*Fraction*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FDIPauseEvent, bool /*bPaused*/);

/**
 * Event hub (Observer). The game mode announces what happened, as it is shown to the player; presenters (audio, HUD,
 * and later achievements, analytics, leaderboards) subscribe here instead of being called by the game mode.
 * Typed native delegates: compile-checked, no plugin. Get it with UDIGameEvents::Get(this).
 */
UCLASS()
class DOUBLEIT_API UDIGameEvents : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UDIGameEvents* Get(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UDIGameEvents>() : nullptr;
	}

	/** A piece landed on the ring (or a breeze blew), at a board angle. */
	FDIPieceEvent PiecePlaced;
	/** A minus lifted a piece off the ring. */
	FDIEvent PieceLifted;
	/** One collision of a chain is shown (not sent when a move is fast-forwarded). */
	FDIFusionStepEvent FusionStep;
	/** A chain finished; Depth is its length. */
	FDIChainEvent ChainSettled;
	/** A number the player had never made before. */
	FDIEvent NumberRevealed;
	/** The score just passed the best of this mode (once per run). */
	FDIEvent NewBest;
	FDIEvent GameOver;
	FDIEvent TutorialAdvanced;
	/** The ring on show holds this share of its capacity (0..1); sent whenever the board is shown. */
	FDIFillEvent RingFilled;
	FDIPauseEvent PauseChanged;
	/** Score, board, pause, theme or settings changed: anything showing state should refresh. */
	FDIEvent StateChanged;
};
