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
#endif

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
