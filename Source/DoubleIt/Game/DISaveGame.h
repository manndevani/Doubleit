#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DISaveGame.generated.h"

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
};
