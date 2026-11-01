#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DoubleItGameMode.h"
#include "Components/InputComponent.h"
#include "Misc/CommandLine.h"

ADIPlayerController::ADIPlayerController()
{
	bShowMouseCursor = true;
}

void ADIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindTouch(IE_Pressed, this, &ADIPlayerController::OnTouchPressed);
	InputComponent->BindTouch(IE_Repeat, this, &ADIPlayerController::OnTouchMoved);
	InputComponent->BindTouch(IE_Released, this, &ADIPlayerController::OnTouchReleased);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADIPlayerController::OnClick);
	InputComponent->BindKey(EKeys::Android_Back, IE_Pressed, this, &ADIPlayerController::OnBack);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADIPlayerController::OnBack);
}

void ADIPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
#if !UE_BUILD_SHIPPING
	// Bots tap through SimulateScreenTap; a real cursor resting over their window must not draw a guide.
	static const bool bBot = FCString::Strifind(FCommandLine::Get(), TEXT("DIPlaythrough")) || FCString::Strifind(FCommandLine::Get(), TEXT("DIAutoplay="));
	if (bBot)
	{
		return;
	}
#endif

	// Mouse hover aims; only re-aim when the cursor actually moved so a finger-driven guide is left alone.
	float X, Y;
	if (!bTouchActive && GetMousePosition(X, Y) && !FVector2D(X, Y).Equals(LastMouse, 0.5))
	{
		LastMouse = FVector2D(X, Y);
		HandleScreenAim(LastMouse);
	}
}

void ADIPlayerController::OnBack()
{
	if (ADoubleItGameMode* GM = GetWorld()->GetAuthGameMode<ADoubleItGameMode>())
	{
		GM->TogglePause();
	}
}

void ADIPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		bTouchActive = true;
		HandleScreenAim(FVector2D(Location));
	}
}

void ADIPlayerController::OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1 && bTouchActive)
	{
		HandleScreenAim(FVector2D(Location));
	}
}

void ADIPlayerController::OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1 && bTouchActive)
	{
		bTouchActive = false;
		HandleScreenTap(FVector2D(Location));
	}
}

void ADIPlayerController::OnClick()
{
	float X, Y;
	if (!bTouchActive && GetMousePosition(X, Y))
	{
		HandleScreenTap(FVector2D(X, Y));
	}
}

TOptional<FVector2D> ADIPlayerController::ScreenToBoard(const FVector2D& ScreenPos) const
{
	FVector WorldPos, WorldDir;
	if (!DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldPos, WorldDir))
	{
		return {};
	}
	return ADIRingActor::FromWorld(WorldPos);
}

void ADIPlayerController::HandleScreenAim(const FVector2D& ScreenPos)
{
	if (ADoubleItGameMode* GM = GetWorld()->GetAuthGameMode<ADoubleItGameMode>())
	{
		GM->HandleAim(ScreenToBoard(ScreenPos));
	}
}

void ADIPlayerController::HandleScreenTap(const FVector2D& ScreenPos)
{
	const TOptional<FVector2D> BoardPos = ScreenToBoard(ScreenPos);
	ADoubleItGameMode* GM = GetWorld()->GetAuthGameMode<ADoubleItGameMode>();
	if (BoardPos && GM)
	{
		GM->HandleTap(*BoardPos);
	}
}
