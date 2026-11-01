#include "DoubleItGameMode.h"
#include "DoubleIt.h"
#include "DIAudio.h"
#include "DIPawn.h"
#include "DIPlatform.h"
#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DISaveGame.h"
#include "Core/DIAdaptive.h"
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

namespace
{
	// Move playback timing: the piece lands, then one collision every ~220 ms, then the next centre appears.
	constexpr float FirstCollisionDelay = 0.16f;
	constexpr float ChainStepDelay = 0.22f;
	constexpr float SettleDelay = 0.14f;
}

ADoubleItGameMode::ADoubleItGameMode()
{
	DefaultPawnClass = ADIPawn::StaticClass();
	PlayerControllerClass = ADIPlayerController::StaticClass();
	HUDClass = AHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ADoubleItGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	if (bPlaythrough && Pt.Moves > 0)
	{
		Pt.Frames++;
		Pt.FrameSum += DeltaSeconds;
		Pt.WorstFrame = FMath::Max<double>(Pt.WorstFrame, DeltaSeconds);
	}
#endif
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
	bAutomationRun = FCString::Strifind(FCommandLine::Get(), TEXT("DIAutoplay=")) || FCString::Strifind(FCommandLine::Get(), TEXT("DIPlaythrough"))
		|| FCString::Strifind(FCommandLine::Get(), TEXT("DIShot="));
	// Automation: -DITutorial forces it; -DIAutoplay (screenshots) skips it.
	if (FParse::Param(FCommandLine::Get(), TEXT("DITutorial")) || FCString::Strifind(FCommandLine::Get(), TEXT("DITutorialStep=")))
	{
		bWantTutorial = true;
	}
	else if (FCString::Strifind(FCommandLine::Get(), TEXT("DIAutoplay=")) || FCString::Strifind(FCommandLine::Get(), TEXT("DIPlaythrough")))
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
	FinishPlayback();
	LearnFromRun(); // an abandoned run still teaches us something if it went on long enough

	bInTutorial = false;
	bInputLocked = false;
	bBoardPaused = false;
	CurrentRules = Rules == EDIGameModeRules::Addition ? EDIRuleSet::Addition : EDIRuleSet::Doubling;

	const FDIPlayerProfile Profile = LoadProfile();
	const FDISpawnTuning Tuning = bAutomationRun ? FDISpawnTuning() : DIAdaptive::TuningFor(Profile);
	UE_LOG(LogDoubleIt, Log, TEXT("Run tuning: skill %.2f, plus %.3f, growth every %d moves (%d runs learned)"),
		DIAdaptive::Skill(Profile), Tuning.PlusChance, Tuning.MovesPerRangeStep, Profile.Runs);
	Game = MakeUnique<FDIGame>(CurrentRules, Seed, Tuning);

	RingOffset = 0.f;
	RunStats = FDIRunSummary();
	ThinkSum = 0.0;
	TurnStart = FPlatformTime::Seconds();
	bRunLearned = false;
	RunStartBest = GetBest();
	bBestCelebrated = false;
	HighestEverAtStart = Profile.HighestEver;
	BoardChanged();
}

void ADoubleItGameMode::RestartRun()
{
	StartNewGame(CurrentRules == EDIRuleSet::Addition ? EDIGameModeRules::Addition : EDIGameModeRules::Doubling, FMath::Rand());
}

void ADoubleItGameMode::StartTutorial()
{
	FinishPlayback();
	LearnFromRun();
	bRunLearned = true;
	bInTutorial = true;
	bInputLocked = false;
	bBoardPaused = false;
	CurrentRules = EDIRuleSet::Doubling;
	RingOffset = 0.f;
	Game = MakeUnique<FDIGame>(CurrentRules, 1);
	int32 FirstStep = 0;
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("DITutorialStep="), FirstStep); // automation: jump to a tutorial board
#endif
	Tutorial.Reset(FirstStep);
	Tutorial.SetupStep(*Game);
	BoardChanged();
}

void ADoubleItGameMode::HandleAim(const TOptional<FVector2D>& BoardPos)
{
	if (!RingActor)
	{
		return;
	}
	const int32 Num = Game ? Game->GetRing().Num() : 0;
	const bool bMinus = Game && Game->GetCenter().Kind == EDIPieceKind::Minus;
	if (!BoardPos || !Game || bBoardPaused || bInputLocked || Game->IsGameOver() || DIRingLayout::IsCenter(*BoardPos)
		|| (bMinus && Num == 0))
	{
		RingActor->ClearAim();
		return;
	}

	float Angle;
	if (bMinus)
	{
		Angle = DIRingLayout::TileAngle(DIRingLayout::TileFromPoint(*BoardPos, Num, RingOffset), Num, RingOffset); // the tile it would lift
	}
	else if (Num == 0)
	{
		Angle = DIRingLayout::AngleOf(*BoardPos);
	}
	else
	{
		const int32 Slot = DIRingLayout::SlotFromPoint(*BoardPos, Num, RingOffset);
		Angle = DIRingLayout::TileAngle(Slot == 0 ? Num : Slot, Num, RingOffset) - UE_PI / Num; // the gap between the two neighbours
	}
	RingActor->SetAim(Angle, bMinus);
}

void ADoubleItGameMode::HandleTap(const FVector2D& BoardPos)
{
	if (RingActor)
	{
		RingActor->ClearAim();
	}
	FinishPlayback(); // a tap mid-animation fast-forwards to the settled board first
	if (!Game || bBoardPaused || bInputLocked)
	{
		return;
	}
	if (Game->IsGameOver())
	{
		RestartRun();
		return;
	}

	const TArray<FDIPiece> Before = Game->GetRing();
	const float BeforeOffset = RingOffset;
	const FDIPiece Placed = Game->GetCenter();
	const int32 MovesBefore = Game->GetMoves();
	if (!DIRingLayout::ApplyTap(*Game, BoardPos, RingOffset))
	{
		return;
	}

	if (Game->GetMoves() > MovesBefore)
	{
		NoteMove(Placed, FPlatformTime::Seconds() - TurnStart);
		StartPlayback(Before, BeforeOffset, Placed);
	}
	else
	{
		// Minus lift or convert: no placement, just re-anchor and show.
		RingOffset = DIRingLayout::Reanchor(Before, BeforeOffset, Game->GetRing());
		Audio->PlayLift();
		BoardChanged();
	}
	if (bInTutorial)
	{
		TutorialAfterMove();
	}
}

void ADoubleItGameMode::StartPlayback(const TArray<FDIPiece>& Before, float BeforeOffset, const FDIPiece& Placed)
{
	PlayEvents = Game->GetLastEvents();
	PlaySteps.Reset();
	bPlayRevealed = false;

	FPlayStep& Land = PlaySteps.AddDefaulted_GetRef();
	Land.Ring = Game->GetPlacedRing();
	Land.Offset = DIRingLayout::Reanchor(Before, BeforeOffset, Land.Ring);
	for (int32 i = 0; i < PlayEvents.Num(); ++i)
	{
		const FPlayStep& Prev = PlaySteps.Last();
		FPlayStep Step;
		Step.Ring = PlayEvents[i].RingAfter;
		Step.Offset = DIRingLayout::Reanchor(Prev.Ring, Prev.Offset, Step.Ring);
		Step.EventIndex = i;
		PlaySteps.Add(MoveTemp(Step));
	}
	FPlayStep Settled;
	Settled.Ring = Game->GetRing();
	Settled.Offset = DIRingLayout::Reanchor(PlaySteps.Last().Ring, PlaySteps.Last().Offset, Settled.Ring);
	PlaySteps.Add(MoveTemp(Settled));
	RingOffset = PlaySteps.Last().Offset; // hit testing already uses the settled ring

	// Landing feedback now; the rest plays on a timer.
	if (Placed.Kind == EDIPieceKind::Breeze)
	{
		Audio->PlayBreeze();
	}
	else
	{
		const int32 Index = Land.Ring.IndexOfByPredicate([&](const FDIPiece& P) { return P.Id == Placed.Id; });
		Audio->PlayPlaceAt(DIRingLayout::TileAngle(FMath::Max(Index, 0), Land.Ring.Num(), Land.Offset));
	}
	PlayIndex = 0;
	ShowStep(0, true);
	PlayIndex = 1;
	GetWorldTimerManager().SetTimer(PlaybackTimer, this, &ADoubleItGameMode::PlayStep, PlayEvents.Num() > 0 ? FirstCollisionDelay : SettleDelay, false);
}

void ADoubleItGameMode::PlayStep()
{
	if (!IsPlayingMove())
	{
		return;
	}
	const int32 Index = PlayIndex++;
	ShowStep(Index, false);
	if (IsPlayingMove())
	{
		const bool bNextIsCollision = PlaySteps[PlayIndex].EventIndex != INDEX_NONE;
		GetWorldTimerManager().SetTimer(PlaybackTimer, this, &ADoubleItGameMode::PlayStep, bNextIsCollision ? ChainStepDelay : SettleDelay, false);
	}
}

void ADoubleItGameMode::FinishPlayback()
{
	if (!IsPlayingMove())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(PlaybackTimer);
	PlayIndex = PlaySteps.Num();
	ShowStep(PlaySteps.Num() - 1, true);
}

float ADoubleItGameMode::PlaybackSeconds() const
{
	return PlayEvents.Num() > 0 ? FirstCollisionDelay + (PlayEvents.Num() - 1) * ChainStepDelay + SettleDelay : SettleDelay;
}

void ADoubleItGameMode::ShowStep(int32 Index, bool bQuiet)
{
	const FPlayStep& Step = PlaySteps[Index];
	const bool bSettled = Index == PlaySteps.Num() - 1;
	if (bSettled)
	{
		BoardChanged(); // next centre appears, HUD catches up
		TurnStart = FPlatformTime::Seconds();
		const int32 Chain = PlayEvents.Num() > 0 ? PlayEvents.Last().ChainDepth : 0;
		if (Game->IsGameOver())
		{
			Audio->PlayGameOver();
		}
		else if (!bQuiet && Chain >= 2)
		{
			Audio->PlayChainSettle();
		}
		if (bPlayRevealed)
		{
			Audio->PlayReveal(0.05f);
			bPlayRevealed = false;
		}
		if (!bInTutorial && !bBestCelebrated && RunStartBest > 0 && Game->GetScore() > RunStartBest)
		{
			bBestCelebrated = true;
			Audio->PlayNewBest();
			if (HUDWidget)
			{
				HUDWidget->CelebrateBest();
			}
		}
		return;
	}

	const FDIFusionEvent* Event = Step.EventIndex != INDEX_NONE ? &PlayEvents[Step.EventIndex] : nullptr;
	RingActor->ShowBoard(Step.Ring, nullptr, Step.Offset, Event, CurrentRules);
	if (!Event)
	{
		return;
	}

	// A number never made before (past the first few) gets a sun-gold ripple and a chime once the chain settles.
	const int64 RevealFloor = CurrentRules == EDIRuleSet::Doubling ? 32 : 6;
	const bool bReveal = !bInTutorial && !bAutomationRun && Event->ResultValue > HighestEverAtStart && Event->ResultValue >= RevealFloor;
	if (bReveal)
	{
		HighestEverAtStart = Event->ResultValue;
		bPlayRevealed = true;
	}
	const int32 Index2 = Step.Ring.IndexOfByPredicate([&](const FDIPiece& P) { return P.Id == Event->ResultId; });
	RingActor->Impact(DIRingLayout::TileAngle(FMath::Max(Index2, 0), Step.Ring.Num(), Step.Offset),
		FMath::Clamp((Event->ChainDepth - 1) / 3.f, 0.f, 1.f), bReveal);
	if (!bQuiet)
	{
		Audio->PlayChainStep(Event->ChainDepth);
	}
}

void ADoubleItGameMode::NoteMove(const FDIPiece& Placed, double ThinkSeconds)
{
	RunStats.Moves = Game->GetMoves();
	RunStats.PlusesPlaced += Placed.Kind == EDIPieceKind::Plus ? 1 : 0;
	for (const FDIFusionEvent& E : Game->GetLastEvents())
	{
		RunStats.Fusions += E.ChainDepth == 1 ? 1 : 0;
		RunStats.BestChain = FMath::Max(RunStats.BestChain, E.ChainDepth);
	}
	ThinkSum += FMath::Min(ThinkSeconds, 30.0); // a phone set down mid-run should not read as slow thinking
	RunStats.AvgThinkSeconds = float(ThinkSum / FMath::Max(RunStats.Moves, 1));
	RunStats.Highest = Game->GetHighestValue();
}

FName ADoubleItGameMode::ModeKey() const
{
	return CurrentRules == EDIRuleSet::Addition ? FName(TEXT("Addition")) : FName(TEXT("Classic"));
}

FDIPlayerProfile ADoubleItGameMode::LoadProfile() const
{
	const FDIProfileRecord* Record = Save ? Save->Profiles.Find(ModeKey()) : nullptr;
	return Record ? Record->ToProfile() : FDIPlayerProfile();
}

void ADoubleItGameMode::LearnFromRun()
{
	// Finished runs always count; abandoned ones only once they say something (15+ moves).
	if (bRunLearned || bInTutorial || bAutomationRun || !Game || !Save || (!Game->IsGameOver() && RunStats.Moves < 15))
	{
		return;
	}
	bRunLearned = true;
	FDIPlayerProfile Profile = LoadProfile();
	DIAdaptive::Learn(Profile, RunStats);
	Save->Profiles.Add(ModeKey(), FDIProfileRecord::From(Profile));
	PersistSettings();
	UE_LOG(LogDoubleIt, Log, TEXT("Learned run: %d moves, chain %d, skill now %.2f"), RunStats.Moves, RunStats.BestChain, DIAdaptive::Skill(Profile));
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
		FinishPlayback();
		Tutorial.SetupStep(*Game);
		RingOffset = 0.f;
		BoardChanged();
	}), PlaybackSeconds() + (bAdvance ? 0.9f : 0.6f), false);
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
		RingActor->SyncFromGame(*Game, RingOffset);
	}
	if (IsGameOver() && !bInTutorial)
	{
		RecordBest();
		LearnFromRun();
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
	float AimDegrees = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("DIAim="), AimDegrees))
	{
		// Pointer held at this clockwise angle on the ring (after the first mouse-hover tick has run).
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, AimDegrees]()
		{
			const float A = FMath::DegreesToRadians(AimDegrees);
			HandleAim(FVector2D(DIRingLayout::RingRadius * FMath::Sin(A), DIRingLayout::RingRadius * FMath::Cos(A)));
		}), 1.f, false);
	}

	if (FCString::Strifind(FCommandLine::Get(), TEXT("DIPlaythrough")))
	{
		int32 Seed = 12345;
		FParse::Value(FCommandLine::Get(), TEXT("DIPlaythrough="), Seed);
		FParse::Value(FCommandLine::Get(), TEXT("DIShotDir="), PlaythroughDir);
		StartNewGame(EDIGameModeRules::Doubling, Seed);
		bPlaythrough = true;
		UE_LOG(LogDoubleIt, Display, TEXT("Playthrough: start seed=%d"), Seed);
		PlaythroughSchedule(2.f);
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
