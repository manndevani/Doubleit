#include "DoubleItGameMode.h"
#include "DoubleIt.h"
#include "DIAudio.h"
#include "DIGameSettings.h"
#include "DIGameEvents.h"
#include "DIText.h"
#include "DIPawn.h"
#include "DIPlatform.h"
#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "DISaveGame.h"
#include "Core/DIAdaptive.h"
#include "Core/DIPieceType.h"
#include "Core/DIRingLayout.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
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
	Events = UDIGameEvents::Get(this);

	Save = Cast<UDISaveGame>(UGameplayStatics::LoadGameFromSlot(UDISaveGame::SlotName, 0));
	if (!Save)
	{
		Save = Cast<UDISaveGame>(UGameplayStatics::CreateSaveGameObject(UDISaveGame::StaticClass()));
		Save->MarkNew();
	}
	else if (Save->Migrate())
	{
		UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	}

	Audio = NewObject<UDIAudio>(this);
	Audio->SetSoundOn(Save->bSound);
	Audio->bHaptics = Save->bHaptics;
	Audio->SetVolumes(Save->EffectsVolume, Save->AmbienceVolume);
	Audio->Listen(*Events);

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
	// Phones may close a backgrounded app without warning: the run is saved on the way out (and after every move).
	BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &ADoubleItGameMode::SaveRun);
	DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &ADoubleItGameMode::SaveRun);
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
	else if (!ResumeRun())
	{
		StartNewGame(UDIGameSettings::Get().DefaultMode, FMath::Rand());
	}
	UE_LOG(LogDoubleIt, Log, TEXT("Ready"));

#if !UE_BUILD_SHIPPING
	RunAutomationFromCommandLine();
#endif
}

void ADoubleItGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	SaveRun();
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
	FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
	Super::EndPlay(Reason);
}

void ADoubleItGameMode::StartNewGame(FName ModeKey, int32 Seed)
{
	FinishPlayback();
	LearnFromRun(); // an abandoned run still teaches us something if it went on long enough

	bInTutorial = false;
	bInputLocked = false;
	bBoardPaused = false;
	FDIMode Mode = UDIGameSettings::MakeMode(ModeKey);
	CurrentMode = Mode.Key;

	const FDIPlayerProfile Profile = LoadProfile();
	const FDIAdaptiveTuning& Adapt = Mode.Config.Adaptive;
	if (!bAutomationRun)
	{
		Mode.Config.Spawn = DIAdaptive::TuningFor(Profile, Mode.Config.Spawn, Adapt);
	}
	const FDISpawnTuning& Tuning = Mode.Config.Spawn;
	UE_LOG(LogDoubleIt, Log, TEXT("Run tuning: skill %.2f, plus %.3f, growth every %d moves (%d runs learned)"),
		DIAdaptive::Skill(Profile, Adapt), Tuning.ChanceOf(EDIPieceKind::Plus), Tuning.MovesPerRangeStep, Profile.Runs);
	Game = MakeUnique<FDIGame>(Mode, Seed);

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
	StartNewGame(CurrentMode, FMath::Rand());
}

void ADoubleItGameMode::StartTutorial()
{
	FinishPlayback();
	LearnFromRun();
	bRunLearned = true;
	bInTutorial = true;
	bInputLocked = false;
	bBoardPaused = false;
	CurrentMode = DIModes::Classic; // the tutorial boards are written for Classic
	RingOffset = 0.f;
	Game = MakeUnique<FDIGame>(UDIGameSettings::MakeMode(CurrentMode), 1);
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
	const bool bMinus = Game && DIPieces::Of(Game->GetCenter()).LiftsFromRing();
	if (!BoardPos || !Game || bBoardPaused || bInputLocked || Game->IsGameOver() || DIRingLayout::IsCenter(*BoardPos)
		|| (bMinus && Num == 0) || (bInTutorial && Game->CanConvertCenter()))
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
		RingActor->ClearHint(); // back after the move, for whatever the board asks next
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
	if (bInTutorial)
	{
		// Try the tap on a copy first: a move the step does not teach is never played (and so never taken back).
		FDIGame Trial = *Game;
		if (DIRingLayout::ApplyTap(Trial, BoardPos, RingOffset) && Tutorial.Judge(Trial) == EDITutorialResult::Retry)
		{
			Events->TutorialNudge.Broadcast();
			UpdateHint(); // the hint stays where it was
			return;
		}
	}

	const TArray<FDIPiece> Before = Game->GetRing();
	const float BeforeOffset = RingOffset;
	const FDIPiece Placed = Game->GetCenter();
	const int32 MovesBefore = Game->GetMoves();
	if (!DIRingLayout::ApplyTap(*Game, BoardPos, RingOffset))
	{
		UpdateHint();
		return;
	}

	// The tutorial judges the move first, so the board refresh below never hints at the step just finished.
	if (bInTutorial)
	{
		TutorialAfterMove();
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
		Events->PieceLifted.Broadcast();
		BoardChanged();
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
	const int32 Index = Land.Ring.IndexOfByPredicate([&](const FDIPiece& P) { return P.Id == Placed.Id; });
	Events->PiecePlaced.Broadcast(Placed, DIRingLayout::TileAngle(FMath::Max(Index, 0), Land.Ring.Num(), Land.Offset));
	PlayIndex = 0;
	ShowStep(0, true);
	PlayIndex = 1;
	PlayPlacedId = Placed.Id;
	PlayLandDeadline = FPlatformTime::Seconds() + UDIGameSettings::Get().LandWaitMax;
	bPlayLanded = false;
	GetWorldTimerManager().SetTimer(PlaybackTimer, this, &ADoubleItGameMode::PlayStep, StepDelay(PlayIndex), false);
}

void ADoubleItGameMode::PlayStep()
{
	if (!IsPlayingMove())
	{
		return;
	}
	if (WaitingToLand())
	{
		// The first collision starts once the pebble is on the ring, after a short beat.
		const UDIGameSettings& S = UDIGameSettings::Get();
		const bool bLanded = !RingActor || RingActor->HasLanded(PlayPlacedId, S.LandTolerance) || FPlatformTime::Seconds() >= PlayLandDeadline;
		bPlayLanded = bLanded;
		GetWorldTimerManager().SetTimer(PlaybackTimer, this, &ADoubleItGameMode::PlayStep, bLanded ? S.FirstCollisionDelay : S.LandCheckSeconds, false);
		return;
	}
	const int32 Index = PlayIndex++;
	ShowStep(Index, false);
	if (IsPlayingMove())
	{
		GetWorldTimerManager().SetTimer(PlaybackTimer, this, &ADoubleItGameMode::PlayStep, StepDelay(PlayIndex), false);
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

float ADoubleItGameMode::StepDelay(int32 Index) const
{
	const UDIGameSettings& S = UDIGameSettings::Get();
	if (!PlaySteps.IsValidIndex(Index) || PlaySteps[Index].EventIndex == INDEX_NONE)
	{
		return S.SettleDelay;
	}
	return Index == 1 ? S.LandCheckSeconds : S.ChainStepDelay;
}

bool ADoubleItGameMode::WaitingToLand() const
{
	return PlayIndex == 1 && !bPlayLanded && PlaySteps.IsValidIndex(1) && PlaySteps[1].EventIndex != INDEX_NONE;
}

float ADoubleItGameMode::PlaybackSeconds() const
{
	float Total = 0.f;
	int32 From = 1;
	const UDIGameSettings& S = UDIGameSettings::Get();
	if (PlaySteps.IsValidIndex(1) && PlaySteps[1].EventIndex != INDEX_NONE)
	{
		Total += S.LandWaitMax + S.FirstCollisionDelay; // the longest the landing can take
		From = 2;
	}
	for (int32 i = From; i < PlaySteps.Num(); ++i)
	{
		Total += StepDelay(i);
	}
	return FMath::Max(Total, UDIGameSettings::Get().SettleDelay);
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
			Events->GameOver.Broadcast();
		}
		else if (!bQuiet && Chain > 0)
		{
			Events->ChainSettled.Broadcast(Chain);
		}
		if (bPlayRevealed)
		{
			Events->NumberRevealed.Broadcast();
			bPlayRevealed = false;
		}
		if (!bInTutorial && !bBestCelebrated && RunStartBest > 0 && Game->GetScore() > RunStartBest)
		{
			bBestCelebrated = true;
			Events->NewBest.Broadcast();
		}
		return;
	}

	const FDIFusionEvent* Event = Step.EventIndex != INDEX_NONE ? &PlayEvents[Step.EventIndex] : nullptr;
	RingActor->ShowBoard(Step.Ring, nullptr, Step.Offset, Event, Game->GetMode());
	if (!Event)
	{
		return;
	}

	// A number never made before (past the first few) gets a sun-gold ripple and a chime once the chain settles.
	const int64 RevealFloor = Game->GetMode().Config.RevealFrom;
	const bool bReveal = !bInTutorial && !bAutomationRun && Event->ResultValue > HighestEverAtStart && Event->ResultValue >= RevealFloor;
	if (bReveal)
	{
		HighestEverAtStart = Event->ResultValue;
		bPlayRevealed = true;
	}
	const int32 Index2 = Step.Ring.IndexOfByPredicate([&](const FDIPiece& P) { return P.Id == Event->ResultId; });
	const float Angle = DIRingLayout::TileAngle(FMath::Max(Index2, 0), Step.Ring.Num(), Step.Offset);
	RingActor->Impact(Angle, FMath::Clamp((Event->ChainDepth - 1) / 3.f, 0.f, 1.f), bReveal);
	if (!bQuiet)
	{
		Events->FusionStep.Broadcast(*Event, Angle);
	}
}

void ADoubleItGameMode::NoteMove(const FDIPiece& Placed, double ThinkSeconds)
{
	RunStats.Moves = Game->GetMoves();
	RunStats.PlusesPlaced += DIPieces::Of(Placed).IsCatalyst() ? 1 : 0;
	for (const FDIFusionEvent& E : Game->GetLastEvents())
	{
		RunStats.Fusions += E.ChainDepth == 1 ? 1 : 0;
		RunStats.BestChain = FMath::Max(RunStats.BestChain, E.ChainDepth);
	}
	ThinkSum += FMath::Min(ThinkSeconds, double(Game->GetMode().Config.Adaptive.MaxThinkSeconds));
	RunStats.AvgThinkSeconds = float(ThinkSum / FMath::Max(RunStats.Moves, 1));
	RunStats.Highest = Game->GetHighestValue();
}

FName ADoubleItGameMode::ModeKey() const
{
	return CurrentMode;
}

FDIPlayerProfile ADoubleItGameMode::LoadProfile() const
{
	const FDIProfileRecord* Record = Save ? Save->Profiles.Find(ModeKey()) : nullptr;
	return Record ? Record->ToProfile() : FDIPlayerProfile();
}

void ADoubleItGameMode::LearnFromRun()
{
	// Finished runs always count; abandoned ones only once they say something (15+ moves).
	if (bRunLearned || bInTutorial || bAutomationRun || !Game || !Save || (!Game->IsGameOver() && RunStats.Moves < Game->GetMode().Config.Adaptive.MinAbandonedMoves))
	{
		return;
	}
	bRunLearned = true;
	FDIPlayerProfile Profile = LoadProfile();
	const FDIAdaptiveTuning& Adapt = Game->GetMode().Config.Adaptive;
	DIAdaptive::Learn(Profile, RunStats, Adapt);
	Save->Profiles.Add(ModeKey(), FDIProfileRecord::From(Profile));
	PersistSettings();
	UE_LOG(LogDoubleIt, Log, TEXT("Learned run: %d moves, chain %d, skill now %.2f"), RunStats.Moves, RunStats.BestChain, DIAdaptive::Skill(Profile, Adapt));
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
	if (RingActor)
	{
		RingActor->ClearHint();
	}
	const bool bAdvance = Result == EDITutorialResult::Advance;
	if (bAdvance)
	{
		Events->TutorialAdvanced.Broadcast();
	}
	GetWorldTimerManager().SetTimer(TutorialTimer, FTimerDelegate::CreateWeakLambda(this, [this, bAdvance]()
	{
		bInputLocked = false;
		if (Tutorial.IsFinished())
		{
			Save->bTutorialDone = true;
			PersistSettings();
			StartNewGame(UDIGameSettings::Get().DefaultMode, FMath::Rand());
			return;
		}
		FinishPlayback();
		Tutorial.SetupStep(*Game);
		RingOffset = 0.f;
		BoardChanged();
	}), PlaybackSeconds() + (bAdvance ? 0.9f : 0.6f), false);
}

FText ADoubleItGameMode::GetTutorialText() const
{
	const FString Key = Tutorial.GetInstructionKey();
	return Key.IsEmpty() ? FText::GetEmpty() : DIText::Get(Key);
}

FText ADoubleItGameMode::GetTutorialProgress() const
{
	return FText::Format(DIText::Get(TEXT("HUD.TutorialProgress")),
		FMath::Min(Tutorial.GetStep() + 1, FDITutorial::NumSteps), FDITutorial::NumSteps);
}

void ADoubleItGameMode::SetBoardPaused(bool bPaused)
{
	bBoardPaused = bPaused;
	Events->PauseChanged.Broadcast(bPaused);
	Events->StateChanged.Broadcast();
}

void ADoubleItGameMode::ToggleSound()
{
	Save->bSound = !Save->bSound;
	Audio->SetSoundOn(Save->bSound);
	PersistSettings();
}

void ADoubleItGameMode::SetEffectsVolume(float Volume, bool bPersist)
{
	Save->EffectsVolume = FMath::Clamp(Volume, 0.f, 1.f);
	Audio->SetVolumes(Save->EffectsVolume, Save->AmbienceVolume);
	if (bPersist)
	{
		Audio->Play(TEXT("TutorialStep")); // a sample at the new level
		PersistSettings();
	}
}

void ADoubleItGameMode::SetAmbienceVolume(float Volume, bool bPersist)
{
	Save->AmbienceVolume = FMath::Clamp(Volume, 0.f, 1.f);
	Audio->SetVolumes(Save->EffectsVolume, Save->AmbienceVolume);
	if (bPersist)
	{
		PersistSettings();
	}
}

void ADoubleItGameMode::ToggleHaptics()
{
	Save->bHaptics = !Save->bHaptics;
	Audio->bHaptics = Save->bHaptics;
	if (Save->bHaptics)
	{
		Audio->Play(TEXT("HapticsOn")); // a tick so the player feels what they turned on
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
	Events->StateChanged.Broadcast();
}

void ADoubleItGameMode::OnAppForeground()
{
	ApplyTheme();
}

namespace
{
	/** Layout of UDISaveGame::RunData around FDIGame::SerializeRun. Bump when the stats below change. */
	constexpr int32 RunRecordFormat = 1;
}

void ADoubleItGameMode::SaveRun()
{
	if (!Save || bAutomationRun)
	{
		return; // bots and screenshots never touch the player.s run
	}
	if (!Game || bInTutorial || Game->IsGameOver())
	{
		if (Save->RunData.Num() == 0)
		{
			return;
		}
		Save->RunData.Reset();
	}
	else
	{
		Save->RunData.Reset();
		FMemoryWriter Ar(Save->RunData);
		int32 Format = RunRecordFormat;
		Ar << Format;
		Game->SerializeRun(Ar);
		Ar << RunStats << ThinkSum << RunStartBest << HighestEverAtStart << bBestCelebrated;
		Save->RunMode = CurrentMode;
	}
	UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
}

bool ADoubleItGameMode::ResumeRun()
{
	if (!Save || bAutomationRun || Save->RunData.Num() == 0)
	{
		return false;
	}
	TUniquePtr<FDIGame> Loaded = MakeUnique<FDIGame>(UDIGameSettings::MakeMode(Save->RunMode), 0);
	FMemoryReader Ar(Save->RunData);
	int32 Format = 0;
	Ar << Format;
	FDIRunSummary Stats;
	double Think = 0.0;
	int64 StartBest = 0, HighestStart = 0;
	bool bCelebrated = false;
	bool bOk = Format == RunRecordFormat && Loaded->SerializeRun(Ar);
	if (bOk)
	{
		Ar << Stats << Think << StartBest << HighestStart << bCelebrated;
		bOk = !Ar.IsError() && !Loaded->IsGameOver();
	}
	if (!bOk)
	{
		UE_LOG(LogDoubleIt, Warning, TEXT("Saved run could not be read (format %d); starting a new one"), Format);
		Save->RunData.Reset();
		return false;
	}

	FinishPlayback();
	bInTutorial = false;
	bInputLocked = false;
	bBoardPaused = false;
	Game = MoveTemp(Loaded);
	CurrentMode = Game->GetMode().Key;
	RingOffset = 0.f;
	RunStats = Stats;
	ThinkSum = Think;
	TurnStart = FPlatformTime::Seconds();
	bRunLearned = false;
	RunStartBest = StartBest;
	bBestCelebrated = bCelebrated;
	HighestEverAtStart = HighestStart;
	UE_LOG(LogDoubleIt, Log, TEXT("Resumed run: %d moves, score %lld"), Game->GetMoves(), Game->GetScore());
	BoardChanged();
	return true;
}

int64 ADoubleItGameMode::GetBest() const
{
	if (!Save)
	{
		return 0;
	}
	const int64 Stored = Save->Bests.FindRef(ModeKey());
	return FMath::Max(Stored, bInTutorial ? 0 : GetScore());
}

void ADoubleItGameMode::UpdateHint()
{
	if (!RingActor)
	{
		return;
	}
	const TOptional<FDITapTarget> Target = bInTutorial && !bInputLocked && Game ? Tutorial.FindTarget(*Game, RingOffset) : TOptional<FDITapTarget>();
	if (Target)
	{
		RingActor->SetHint(Target->Pos, Target->Size);
	}
	else
	{
		RingActor->ClearHint();
	}
}

void ADoubleItGameMode::BoardChanged()
{
	if (RingActor && Game)
	{
		RingActor->SyncFromGame(*Game, RingOffset);
		Events->RingFilled.Broadcast(float(Game->GetRing().Num()) / FMath::Max(Game->GetMaxRingPieces(), 1));
	}
	UpdateHint();
	if (IsGameOver() && !bInTutorial)
	{
		RecordBest();
		LearnFromRun();
	}
	SaveRun();
	Events->StateChanged.Broadcast();
}

void ADoubleItGameMode::RecordBest()
{
	int64& Best = Save->Bests.FindOrAdd(ModeKey());
	if (GetScore() > Best)
	{
		Best = GetScore();
		PersistSettings();
	}
}

void ADoubleItGameMode::PersistSettings()
{
	UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	Events->StateChanged.Broadcast();
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
	if (FParse::Param(FCommandLine::Get(), TEXT("DIGlyphBoard")))
	{
		// Every digit and piece kind on one board, for checking tile art.
		TArray<FDIPiece> Ring;
		for (int64 V : { 1024, 2048, 4096, 8192, 16384, 32768, 65536, 512, 256, 128, 64, 32, 16 })
		{
			Ring.Add(FDIPiece::Number(V));
		}
		Ring.Add(FDIPiece::Plus());
		Ring.Add(FDIPiece::Minus());
		Ring.Add(FDIPiece::Breeze());
		Game->SetBoard(Ring, FDIPiece::Number(90));
		RingOffset = 0.f;
		BoardChanged();
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
		StartNewGame(UDIGameSettings::Get().DefaultMode, Seed);
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
