#include "DIPawn.h"
#include "DoubleIt.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ADIPawn::ADIPawn()
{
	PrimaryActorTick.bCanEverTick = true; // only watches for viewport size changes (rotation, fold, split screen)

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// The ring sits on the world origin; FitToViewport moves the pawn along world X to centre it between the HUD bands.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetRelativeLocation(FVector(0, 0, 1000));
	Camera->SetRelativeRotation(FRotator(-90, 0, 0));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(820.f);
	Camera->bConstrainAspectRatio = false;
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;

	// Unlit colours should land on screen as authored: fixed exposure, no tone curve.
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.f;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	PP.bOverride_ToneCurveAmount = true;
	PP.ToneCurveAmount = 0.f;
}

void ADIPawn::BeginPlay()
{
	Super::BeginPlay();
	// The board is built around the world origin; ignore wherever the map spawned us.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}

void ADIPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
		const float UIScale = UWidgetLayoutLibrary::GetViewportScale(this); // settles a frame or two after a resize
		if ((Size != FittedSize || !FMath::IsNearlyEqual(UIScale, FittedUIScale)) && Size.X > 0 && Size.Y > 0)
		{
			FitToViewport(Size);
		}
	}
}

void ADIPawn::FitToViewport(const FIntPoint& Size)
{
	FittedSize = Size;
	FittedUIScale = UWidgetLayoutLibrary::GetViewportScale(this);

	// The board goes in the free band between the top HUD (score, best, tutorial lines) and the game-over lines at 80%
	// height: centred in that band, and zoomed so the ring fits it. 820 world units across is the phone layout; we only
	// ever zoom out from it (squat screens, foldables), and big tablets are capped so stones don't get huge.
	constexpr float PhoneWidth = 820.f;
	constexpr float RingExtent = 345.f;    // ring radius + half a tile + a little air, world units
	constexpr float TopBandUnits = 500.f;  // score, best and two tutorial lines (+ air), UMG units from the top
	constexpr float BottomBand = 0.8f;     // game-over lines start at 80% height
	constexpr float MaxPixelsPerUnit = 1.6f;

	const float W = float(Size.X);
	const float H = float(Size.Y);
	const float UIScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const float BandTop = FMath::Min(TopBandUnits * UIScale, H * 0.45f);
	const float BandBottom = FMath::Max(H * BottomBand - 12.f, BandTop + H * 0.3f);
	const float Room = (BandBottom - BandTop) * 0.5f;
	const float CentreY = BandTop + Room; // pixels from the top

	float OrthoWidth = FMath::Max(PhoneWidth, RingExtent * W / Room);
	OrthoWidth = FMath::Max(OrthoWidth, W / MaxPixelsPerUnit);
	Camera->SetOrthoWidth(OrthoWidth);

	// Move the camera up (board +Y = world +X) so the ring centre lands at CentreY on screen.
	const float UnitsPerPixel = OrthoWidth / W;
	SetActorLocation(FVector((CentreY - H * 0.5f) * UnitsPerPixel, 0.f, 0.f));
	UE_LOG(LogDoubleIt, Log, TEXT("Camera fit: viewport %dx%d, UI scale %.3f, band %.0f-%.0f px, ortho width %.0f"),
		Size.X, Size.Y, UIScale, BandTop, BandBottom, OrthoWidth);
}
