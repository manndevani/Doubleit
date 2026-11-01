#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DoubleItGameMode.h"
#include "Components/InputComponent.h"

ADIPlayerController::ADIPlayerController()
{
	bShowMouseCursor = true;
}

void ADIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindTouch(IE_Pressed, this, &ADIPlayerController::OnTouch);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADIPlayerController::OnClick);
}

void ADIPlayerController::OnTouch(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		HandleScreenTap(FVector2D(Location));
	}
}

void ADIPlayerController::OnClick()
{
	float X, Y;
	if (GetMousePosition(X, Y))
	{
		HandleScreenTap(FVector2D(X, Y));
	}
}

void ADIPlayerController::HandleScreenTap(const FVector2D& ScreenPos)
{
	FVector WorldPos, WorldDir;
	if (!DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldPos, WorldDir))
	{
		return;
	}
	if (ADoubleItGameMode* GM = GetWorld()->GetAuthGameMode<ADoubleItGameMode>())
	{
		GM->HandleTap(ADIRingActor::FromWorld(WorldPos));
	}
}
