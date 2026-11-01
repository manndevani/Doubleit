#include "DoubleItGameMode.h"
#include "DoubleIt.h"
#include "DIAudio.h"
#include "DIPawn.h"
#include "DIPlatform.h"
#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DISaveGame.h"
#include "Core/DIRingLayout.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "TimerManager.h"
#include "UI/DIHUDWidget.h"

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
	HUDClass = AHUD::StaticClass();
}

void ADoubleItGameMode::BeginPlay()
{
	Super::BeginPlay();

	Save = Cast<UDISaveGame>(UGameplayStatics::LoadGameFromSlot(UDISaveGame::SlotName, 0));
	if (!Save)
	{
		Save = Cast<UDISaveGame>(UGameplayStatics::CreateSaveGameObject(UDISaveGame::StaticClass()));
	}

	Audio = NewObject<UDIAudio>(this);
	Audio->bSound = Save->bSound;
	Audio->bHaptics = Save->bHaptics;

	RingActor = GetWorld()->SpawnActor<ADIRingActor>();

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		HUDWidget = CreateWidget<UDIHUDWidget>(PC, UDIHUDWidget::StaticClass());
		HUDWidget->Bind(this);
		HUDWidget->AddToViewport();
		HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}

	ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &ADoubleItGameMode::OnAppForeground);
	ApplyTheme();

	bool bWantTutorial = !Save->bTutorialDone;
#if !UE_BUILD_SHIPPING
	// Automation: -DITutorial forces it; -DIAutoplay (screenshots) skips it.
	if (FParse::Param(FCommandLine::Get(), TEXT("DITutorial")))
	{
		bWantTutorial = true;
	}
	else if (FCString::Strifind(FCommandLine::Get(), TEXT("DIAutoplay=")))
	{
		bWantTutorial = false;
	}
#endif
	if (bWantTutorial)
	{
		StartTutorial();
	}
	else
	{
		StartNewGame(EDIGameModeRules::Doubling, FMath::Rand());
	}
	UE_LOG(LogDoubleIt, Log, TEXT("Ready"));

#if !UE_BUILD_SHIPPING
	RunAutomationFromCommandLine();
#endif
}

void ADoubleItGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
	Super::EndPlay(Reason);
}

void ADoubleItGameMode::StartNewGame(EDIGameModeRules Rules, int32 Seed)
{
	bInTutorial = false;
	bInputLocked = false;
	bBoardPaused = false;
	CurrentRules = Rules == EDIGameModeRules::Addition ? EDIRuleSet::Addition : EDIRuleSet::Doubling;
	Game = MakeUnique<FDIGame>(CurrentRules, Seed);
	BoardChanged();
}

void ADoubleItGameMode::RestartRun()
{
	StartNewGame(CurrentRules == EDIRuleSet::Addition ? EDIGameModeRules::Addition : EDIGameModeRules::Doubling, FMath::Rand());
}

void ADoubleItGameMode::StartTutorial()
{
	bInTutorial = true;
	bInputLocked = false;
	bBoardPaused = false;
	CurrentRules = EDIRuleSet::Doubling;
	Game = MakeUnique<FDIGame>(CurrentRules, 1);
	Tutorial.Reset();
	Tutorial.SetupStep(*Game);
	BoardChanged();
}

void ADoubleItGameMode::HandleTap(const FVector2D& BoardPos)
{
	if (!Game || bBoardPaused || bInputLocked)
	{
		return;
	}
	if (Game->IsGameOver())
	{
		RestartRun();
		return;
	}

	const int32 RingBefore = Game->GetRing().Num();
	if (!DIRingLayout::ApplyTap(*Game, BoardPos))
	{
		return;
	}
	PlayFeedback(RingBefore);
	BoardChanged();
	if (bInTutorial)
	{
		TutorialAfterMove();
	}
}

void ADoubleItGameMode::PlayFeedback(int32 RingBefore)
{
	if (Game->IsGameOver())
	{
		Audio->PlayGameOver();
	}
	else if (Game->GetLastEvents().Num() > 0 && Game->GetRing().Num() < RingBefore)
	{
		Audio->PlayFusion(Game->GetLastEvents().Last().ChainDepth);
	}
	else
	{
		Audio->PlayPlace();
	}
}

void ADoubleItGameMode::TutorialAfterMove()
{
	const EDITutorialResult Result = Tutorial.Evaluate(*Game);
	if (Result == EDITutorialResult::Continue)
	{
		return;
	}

	// Let the move's animation breathe before the next scripted board appears.
	bInputLocked = true;
	const bool bAdvance = Result == EDITutorialResult::Advance;
	if (bAdvance)
	{
		Audio->PlayTutorialStep();
	}
	GetWorldTimerManager().SetTimer(TutorialTimer, FTimerDelegate::CreateWeakLambda(this, [this, bAdvance]()
	{
		bInputLocked = false;
		if (Tutorial.IsFinished())
		{
			Save->bTutorialDone = true;
			PersistSettings();
			StartNewGame(EDIGameModeRules::Doubling, FMath::Rand());
			return;
		}
		Tutorial.SetupStep(*Game);
		BoardChanged();
	}), bAdvance ? 1.1f : 0.8f, false);
}

FString ADoubleItGameMode::GetTutorialProgress() const
{
	return FString::Printf(TEXT("%d / %d"), FMath::Min(Tutorial.GetStep() + 1, FDITutorial::NumSteps), FDITutorial::NumSteps);
}

void ADoubleItGameMode::SetBoardPaused(bool bPaused)
{
	bBoardPaused = bPaused;
	if (HUDWidget)
	{
		HUDWidget->Refresh();
	}
}

void ADoubleItGameMode::ToggleSound()
{
	Save->bSound = !Save->bSound;
	Audio->bSound = Save->bSound;
	PersistSettings();
}

void ADoubleItGameMode::ToggleHaptics()
{
	Save->bHaptics = !Save->bHaptics;
	Audio->bHaptics = Save->bHaptics;
	if (Save->bHaptics)
	{
		DIPlatform::Vibrate(20);
	}
	PersistSettings();
}

void ADoubleItGameMode::CycleTheme()
{
	Save->ThemeMode = EDIThemeMode((uint8(Save->ThemeMode) + 1) % 3);
	PersistSettings();
	ApplyTheme();
}

bool ADoubleItGameMode::IsDarkTheme() const
{
	if (!Save || Save->ThemeMode == EDIThemeMode::System)
	{
		return DIPlatform::IsSystemDarkMode();
	}
	return Save->ThemeMode == EDIThemeMode::Dark;
}

void ADoubleItGameMode::ApplyTheme()
{
	if (RingActor)
	{
		RingActor->SetDark(IsDarkTheme());
	}
	if (HUDWidget)
	{
		HUDWidget->Refresh();
	}
}

void ADoubleItGameMode::OnAppForeground()
{
	ApplyTheme();
}

int64 ADoubleItGameMode::GetBest() const
{
	if (!Save)
	{
		return 0;
	}
	const int64 Stored = CurrentRules == EDIRuleSet::Addition ? Save->BestAddition : Save->BestDoubling;
	return FMath::Max(Stored, bInTutorial ? 0 : GetScore());
}

void ADoubleItGameMode::BoardChanged()
{
	if (RingActor && Game)
	{
		RingActor->SyncFromGame(*Game);
	}
	if (IsGameOver() && !bInTutorial)
	{
		RecordBest();
	}
	if (HUDWidget)
	{
		HUDWidget->Refresh();
	}
	OnBoardChanged.Broadcast();
}

void ADoubleItGameMode::RecordBest()
{
	int64& Best = CurrentRules == EDIRuleSet::Addition ? Save->BestAddition : Save->BestDoubling;
	if (GetScore() > Best)
	{
		Best = GetScore();
		PersistSettings();
	}
}

void ADoubleItGameMode::PersistSettings()
{
	UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	if (HUDWidget)
	{
		HUDWidget->Refresh();
	}
}

#if !UE_BUILD_SHIPPING
void ADoubleItGameMode::RunAutomationFromCommandLine()
{
	int32 Moves = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("DIAutoplay="), Moves) && Game && !bInTutorial)
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

	FString Theme;
	if (FParse::Value(FCommandLine::Get(), TEXT("DITheme="), Theme))
	{
		Save->ThemeMode = Theme == TEXT("dark") ? EDIThemeMode::Dark : EDIThemeMode::Light;
		ApplyTheme();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("DIPaused")))
	{
		SetBoardPaused(true);
	}

	if (FParse::Value(FCommandLine::Get(), TEXT("DIShot="), ScreenshotPath))
	{
		GetWorldTimerManager().SetTimer(AutomationTimer, this, &ADoubleItGameMode::TakeScreenshotWhenReady, 2.0f, false);
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
