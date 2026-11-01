#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DIGame.h"
#include "DoubleItGameMode.generated.h"

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
	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	void StartNewGame(EDIGameModeRules Rules, int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	bool PlaceAt(int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	bool AbsorbAt(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "DoubleIt")
	bool ConvertCenterToPlus();

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	int64 GetScore() const { return Game ? Game->GetScore() : 0; }

	UFUNCTION(BlueprintPure, Category = "DoubleIt")
	bool IsGameOver() const { return Game && Game->IsGameOver(); }

	const FDIGame* GetGame() const { return Game.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "DoubleIt")
	FDIOnBoardChanged OnBoardChanged;

protected:
	virtual void BeginPlay() override;

private:
	bool Broadcast(bool bChanged);

	TUniquePtr<FDIGame> Game;
};
