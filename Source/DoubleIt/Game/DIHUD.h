#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DIHUD.generated.h"

/** Prototype HUD drawn on the canvas (score, best, game over). Replaced by UMG in M2. */
UCLASS()
class DOUBLEIT_API ADIHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color);
};
