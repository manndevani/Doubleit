#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DIPlayerController.generated.h"

/** Turns clicks/touches into board-space taps for the game mode. */
UCLASS()
class DOUBLEIT_API ADIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADIPlayerController();

protected:
	virtual void SetupInputComponent() override;

private:
	void OnTouch(ETouchIndex::Type FingerIndex, FVector Location);
	void OnClick();
	void OnBack();
	void HandleScreenTap(const FVector2D& ScreenPos);
};
