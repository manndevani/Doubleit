#include "DoubleItGameMode.h"
#include "DoubleIt.h"
#include "DIHUD.h"
#include "DIPawn.h"
#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DISaveGame.h"
#include "Core/DIRingLayout.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif
#endif

ADoubleItGameMode::ADoubleItGameMode()
{
	DefaultPawnClass = ADIPawn::StaticClass();
	PlayerControllerClass = ADIPlayerController::StaticClass();
	HUDClass = ADIHUD::StaticClass();
}

void ADoubleItGameMode::BeginPlay()
{
	Super::BeginPlay();

	Save = Cast<UDISaveGame>(UGameplayStatics::LoadGameFromSlot(UDISaveGame::SlotName, 0));
	if (!Save)
	{
		Save = Cast<UDISaveGame>(UGameplayStatics::CreateSaveGameObject(UDISaveGame::StaticClass()));
	}

	RingActor = GetWorld()->SpawnActor<ADIRingActor>();
	StartNewGame(EDIGameModeRules::Doubling, FMath::Rand());
	UE_LOG(LogDoubleIt, Log, TEXT("Ready"));

#if !UE_BUILD_SHIPPING
	RunAutomationFromCommandLine();
#endif
}

void ADoubleItGameMode::StartNewGame(EDIGameModeRules Rules, int32 Seed)
{
	CurrentRules = Rules == EDIGameModeRules::Addition ? EDIRuleSet::Addition : EDIRuleSet::Doubling;
	Game = MakeUnique<FDIGame>(CurrentRules, Seed);
	BoardChanged();
}

void ADoubleItGameMode::HandleTap(const FVector2D& BoardPos)
{
	if (!Game)
	{
		return;
	}
	if (Game->IsGameOver())
	{
		StartNewGame(CurrentRules == EDIRuleSet::Addition ? EDIGameModeRules::Addition : EDIGameModeRules::Doubling, FMath::Rand());
		return;
	}
	if (DIRingLayout::ApplyTap(*Game, BoardPos))
	{
		BoardChanged();
	}
}

int64 ADoubleItGameMode::GetBest() const
{
	if (!Save)
	{
		return 0;
	}
	const int64 Stored = CurrentRules == EDIRuleSet::Addition ? Save->BestAddition : Save->BestDoubling;
	return FMath::Max(Stored, GetScore());
}

void ADoubleItGameMode::BoardChanged()
{
	if (RingActor && Game)
	{
		RingActor->SyncFromGame(*Game);
	}
	if (IsGameOver())
	{
		RecordBest();
	}
	OnBoardChanged.Broadcast();
}

void ADoubleItGameMode::RecordBest()
{
	if (!Save)
	{
		return;
	}
	int64& Best = CurrentRules == EDIRuleSet::Addition ? Save->BestAddition : Save->BestDoubling;
	if (GetScore() > Best)
	{
		Best = GetScore();
		UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	}
}

#if !UE_BUILD_SHIPPING
void ADoubleItGameMode::RunAutomationFromCommandLine()
{
	int32 Moves = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("DIAutoplay="), Moves) && Game)
	{
		FRandomStream Bot(7);
		for (int32 i = 0; i < Moves && !Game->IsGameOver(); ++i)
		{
			const float Angle = Bot.FRandRange(0.f, UE_TWO_PI);
			const FVector2D OnRing(DIRingLayout::RingRadius * FMath::Sin(Angle), DIRingLayout::RingRadius * FMath::Cos(Angle));
			DIRingLayout::ApplyTap(*Game, OnRing);
		}
		BoardChanged();
	}

	if (FParse::Value(FCommandLine::Get(), TEXT("DIShot="), ScreenshotPath))
	{
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &ADoubleItGameMode::TakeScreenshotWhenReady, 1.5f, false);
	}
}

void ADoubleItGameMode::TakeScreenshotWhenReady()
{
#if WITH_EDITOR
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &ADoubleItGameMode::TakeScreenshotWhenReady, 1.f, false);
		return;
	}
#endif
	if (!ScreenshotPath.IsEmpty())
	{
		FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
		UE_LOG(LogDoubleIt, Log, TEXT("Screenshot requested: %s"), *ScreenshotPath);
		ScreenshotPath.Empty();
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &ADoubleItGameMode::TakeScreenshotWhenReady, 1.f, false);
		return;
	}
	FPlatformMisc::RequestExit(false, TEXT("DIShot"));
}
#endif
