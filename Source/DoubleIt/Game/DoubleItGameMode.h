#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DIGame.h"
#include "DoubleItGameMode.generated.h"

class ADIRingActor;
class UDISaveGame;

UENUM(BlueprintType)
enum class EDIGameModeRules : uint8
{
	Doubling,
	Addition
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDIOnBoardChanged);

/** Owns the current run and exposes the minimal API that UI/rendering need. */
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

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	int64 GetScore() const { return Game ? Game->GetScore() : 0; }

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	int64 GetBest() const;

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	bool IsGameOver() const { return Game && Game->IsGameOver(); }

	const FDIGame* GetGame() const { return Game.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "DoubleIt")
	FDIOnBoardChanged OnBoardChanged;

protected:
	virtual void BeginPlay() override;

private:
	void BoardChanged();
	void RecordBest();

#if !UE_BUILD_SHIPPING
	/** -DIAutoplay=N plays N random taps; -DIShot=<file.png> saves a screenshot then quits. */
	void RunAutomationFromCommandLine();
	void TakeScreenshotWhenReady();
	FString ScreenshotPath;
	FTimerHandle AutomationTimer;
#endif

	TUniquePtr<FDIGame> Game;
	EDIRuleSet CurrentRules = EDIRuleSet::Doubling;

	UPROPERTY()
	TObjectPtr<ADIRingActor> RingActor;

	UPROPERTY()
	TObjectPtr<UDISaveGame> Save;
};
