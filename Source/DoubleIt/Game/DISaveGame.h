#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DISaveGame.generated.h"

UENUM()
enum class EDIThemeMode : uint8
{
	System,
	Light,
	Dark
};

UCLASS()
class DOUBLEIT_API UDISaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("DoubleIt");

	UPROPERTY()
	int64 BestDoubling = 0;

	UPROPERTY()
	int64 BestAddition = 0;

	UPROPERTY()
	bool bSound = true;

	UPROPERTY()
	bool bHaptics = true;

	UPROPERTY()
	EDIThemeMode ThemeMode = EDIThemeMode::System;

	UPROPERTY()
	bool bTutorialDone = false;
};
