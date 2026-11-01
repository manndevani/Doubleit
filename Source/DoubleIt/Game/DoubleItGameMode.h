#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DIGame.h"
#include "Core/DITutorial.h"
#include "DoubleItGameMode.generated.h"

class ADIRingActor;
class UDIAudio;
class UDIHUDWidget;
class UDISaveGame;

UENUM(BlueprintType)
enum class EDIGameModeRules : uint8
{
	Doubling,
	Addition
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDIOnBoardChanged);

/** Owns the current run, tutorial, settings and the objects that present them (ring actor, HUD, audio). */
UCLASS()
class DOUBLEIT_API ADoubleItGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADoubleItGameMode();

	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	void StartNewGame(EDIGameModeRules Rules, int32 Seed);

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
	bool IsInTutorial() const { return bInTutorial; }
	bool IsDarkTheme() const;
	FString GetTutorialText() const { return Tutorial.GetInstruction(); }
	FString GetTutorialProgress() const;
	const UDISaveGame* GetSave() const { return Save; }
	const FDIGame* GetGame() const { return Game.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "DoubleIt")
	FDIOnBoardChanged OnBoardChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void BoardChanged();
	void RecordBest();
	void PersistSettings();
	void ApplyTheme();
	void PlayFeedback(int32 RingBefore);
	void TutorialAfterMove();
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
	EDIRuleSet CurrentRules = EDIRuleSet::Doubling;
	bool bInTutorial = false;
	bool bInputLocked = false;
	bool bBoardPaused = false;
	FTimerHandle TutorialTimer;
	FDelegateHandle ForegroundHandle;

	UPROPERTY()
	TObjectPtr<ADIRingActor> RingActor;

	UPROPERTY()
	TObjectPtr<UDISaveGame> Save;

	UPROPERTY()
	TObjectPtr<UDIAudio> Audio;

	UPROPERTY()
	TObjectPtr<UDIHUDWidget> HUDWidget;
};
