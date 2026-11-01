#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DIGame.h"
#include "Core/DITutorial.h"
#include "Core/DIAdaptive.h"
#include "DoubleItGameMode.generated.h"

class ADIRingActor;
class UDIAudio;
class UDIHUDWidget;
class UDISaveGame;

/** Owns the current run, tutorial, settings and the objects that present them (ring actor, HUD, audio). */
UCLASS()
class DOUBLEIT_API ADoubleItGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADoubleItGameMode();

	/** Starts a run of the mode registered as ModeKey (see Core/DIMode.h), with the player's adaptive tuning. */
	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	void StartNewGame(FName ModeKey, int32 Seed);

	/** Tap in board space (see DIRingLayout). Restarts the run after game over. */
	void HandleTap(const FVector2D& BoardPos);
	/** Shows the radial aim guide for a pointer at BoardPos (or hides it when unset). */
	void HandleAim(const TOptional<FVector2D>& BoardPos);

	void RestartRun();
	void StartTutorial();
	void SetBoardPaused(bool bPaused);
	void TogglePause() { SetBoardPaused(!bBoardPaused); }
	void ToggleSound();
	void ToggleHaptics();
	void CycleTheme();

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	int64 GetScore() const { return Game ? Game->GetScore() : 0; }

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	int64 GetBest() const;

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	bool IsGameOver() const { return Game && Game->IsGameOver(); }

	bool IsBoardPaused() const { return bBoardPaused; }
	/** A move is still being shown step by step (taps fast-forward it). */
	bool IsPlayingMove() const { return PlayIndex < PlaySteps.Num(); }
	/** Current ring rotation, radians (see DIRingLayout::Reanchor). */
	float GetRingOffset() const { return RingOffset; }
	bool IsInTutorial() const { return bInTutorial; }
	bool IsDarkTheme() const;
	FText GetTutorialText() const;
	FText GetTutorialProgress() const;
	const UDISaveGame* GetSave() const { return Save; }
	const FDIGame* GetGame() const { return Game.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void BoardChanged();
	void RecordBest();
	void PersistSettings();
	void ApplyTheme();
	void TutorialAfterMove();

	/**
	 * Move playback: FDIGame resolves a move instantly; it is then shown in steps (piece lands, each chain collision
	 * ~220 ms apart, next centre appears). Each step re-anchors the ring so motion is symmetric around the action.
	 */
	struct FPlayStep
	{
		TArray<FDIPiece> Ring;
		float Offset = 0.f;
		int32 EventIndex = INDEX_NONE; // fusion event shown by this step
	};
	void StartPlayback(const TArray<FDIPiece>& Before, float BeforeOffset, const FDIPiece& Placed);
	void PlayStep();
	void ShowStep(int32 Index, bool bQuiet);
	void FinishPlayback();
	float StepDelay(int32 Index) const;
	float PlaybackSeconds() const;

	/** Adaptive pace: learn from finished runs, tune the next run's spawner. */
	FName ModeKey() const;
	FDIPlayerProfile LoadProfile() const;
	void LearnFromRun();
	void NoteMove(const FDIPiece& Placed, double ThinkSeconds);
	void OnAppForeground();

#if !UE_BUILD_SHIPPING
	/** -DIAutoplay=N plays N random taps; -DIShot=<file.png> saves a screenshot then quits; -DITutorial / -DIPaused show those states. */
	void RunAutomationFromCommandLine();
	void TakeScreenshotWhenReady();
	FString ScreenshotPath;
	FTimerHandle AutomationTimer;

	/** -DIPlaythrough[=Seed] -DIShotDir=<dir>: a bot plays a full run through real screen taps (DoubleItGameMode_Playthrough.cpp). */
	void PlaythroughStep();
	void PlaythroughFinish();
	FVector2D PlaythroughChooseTap() const;
	bool PlaythroughTap(const FVector2D& BoardPos);
	void PlaythroughShot(const FString& Label, float Delay = 0.f);
	void PlaythroughSchedule(float Delay);

	struct FPlaythroughStats
	{
		int32 Moves = 0, Rejected = 0, Mismatches = 0, Fusions = 0, MaxChain = 0, Minus = 0, Converts = 0, Shots = 0;
		int32 Frames = 0;
		double FrameSum = 0, WorstFrame = 0, SettleSum = 0, SettleMax = 0, MoveStart = 0;
		int32 Settles = 0;
		int64 FinalScore = 0;
		bool bAwaitingSettle = false, bSawGameOver = false, bRestarted = false;
	};
	bool bPlaythrough = false;
	FString PlaythroughDir;
	FPlaythroughStats Pt;
	FTimerHandle PlaythroughTimer;
#endif

public:
	virtual void Tick(float DeltaSeconds) override;

	TUniquePtr<FDIGame> Game;
	FDITutorial Tutorial;
	FName CurrentMode = DIModes::Classic;
	bool bInTutorial = false;
	bool bInputLocked = false;
	bool bBoardPaused = false;
	FTimerHandle TutorialTimer;
	FDelegateHandle ForegroundHandle;

	float RingOffset = 0.f;
	TArray<FPlayStep> PlaySteps;
	TArray<FDIFusionEvent> PlayEvents;
	int32 PlayIndex = 0;
	FTimerHandle PlaybackTimer;
	bool bPlayRevealed = false;

	FDIRunSummary RunStats;
	double ThinkSum = 0.0;
	double TurnStart = 0.0;
	bool bRunLearned = true;
	bool bAutomationRun = false; // bots and screenshots neither learn nor get adapted
	int64 RunStartBest = 0;
	bool bBestCelebrated = false;
	int64 HighestEverAtStart = 0;

	UPROPERTY()
	TObjectPtr<ADIRingActor> RingActor;

	UPROPERTY()
	TObjectPtr<UDISaveGame> Save;

	UPROPERTY()
	TObjectPtr<class UDIGameEvents> Events;

	UPROPERTY()
	TObjectPtr<UDIAudio> Audio;

	UPROPERTY()
	TObjectPtr<UDIHUDWidget> HUDWidget;
};
