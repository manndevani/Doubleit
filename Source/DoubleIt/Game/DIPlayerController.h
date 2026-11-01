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

#if !UE_BUILD_SHIPPING
	/** Automation: same path as a real touch (deprojection included). */
	void SimulateScreenTap(const FVector2D& ScreenPos) { HandleScreenTap(ScreenPos); }
#endif

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	/** Touch: press and drag aim, release places, so the guide is visible before committing. */
	void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);
	void OnClick();
	void OnBack();
	void HandleScreenTap(const FVector2D& ScreenPos);
	void HandleScreenAim(const FVector2D& ScreenPos);
	TOptional<FVector2D> ScreenToBoard(const FVector2D& ScreenPos) const;

	bool bTouchActive = false;
	FVector2D LastMouse = FVector2D(-1, -1);
};
