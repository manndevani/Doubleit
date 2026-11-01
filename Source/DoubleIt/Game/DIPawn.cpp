#include "DIPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ADIPawn::ADIPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Kept at X=Y=0: M_Background reads camera-relative world position as board space.
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
