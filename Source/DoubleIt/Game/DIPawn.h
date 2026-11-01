#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DIPawn.generated.h"

class UCameraComponent;

/** Orthographic camera looking down on the ring; its zoom adapts to the screen shape. */
UCLASS()
class DOUBLEIT_API ADIPawn : public APawn
{
	GENERATED_BODY()

public:
	ADIPawn();

public:
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Fits the board between the HUD bands on any screen shape (tall phones, tablets, foldables). */
	void FitToViewport(const FIntPoint& Size);
	FIntPoint FittedSize = FIntPoint::ZeroValue;
	float FittedUIScale = 0.f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
};
