#include "DIAudio.h"
#include "DIGameEvents.h"
#include "DIGameSettings.h"
#include "DIPlatform.h"
#include "Core/DIPieceType.h"
#include "Core/DISynth.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

void UDIAudio::Listen(UDIGameEvents& Events)
{
	Events.PiecePlaced.AddUObject(this, &UDIAudio::OnPiecePlaced);
	Events.FusionStep.AddUObject(this, &UDIAudio::OnFusionStep);
	Events.ChainSettled.AddUObject(this, &UDIAudio::OnChainSettled);
	Events.PieceLifted.AddUObject(this, &UDIAudio::Play, FName(TEXT("Lift")), 0);
	Events.NumberRevealed.AddUObject(this, &UDIAudio::Play, FName(TEXT("Reveal")), 0);
	Events.NewBest.AddUObject(this, &UDIAudio::Play, FName(TEXT("NewBest")), 0);
	Events.GameOver.AddUObject(this, &UDIAudio::Play, FName(TEXT("GameOver")), 0);
	Events.TutorialAdvanced.AddUObject(this, &UDIAudio::Play, FName(TEXT("TutorialStep")), 0);
}

void UDIAudio::OnPiecePlaced(const FDIPiece& Piece, float Angle)
{
	const FName Cue = DIPieces::Of(Piece).LandCue();
	const FDISoundCue* Data = UDIGameSettings::Get().Cues.Find(Cue);
	int32 Level = 0;
	if (Data && Data->bLevelFromAngle)
	{
		const int32 Zones = FMath::Max(UDIGameSettings::Get().PlaceZones, 1);
		const float Turn = FMath::Fmod(Angle + 4.f * UE_TWO_PI, UE_TWO_PI) / UE_TWO_PI;
		Level = FMath::Min(FMath::FloorToInt32(Turn * Zones), Zones - 1);
	}
	Play(Cue, Level);
}

void UDIAudio::OnFusionStep(const FDIFusionEvent& Step, float Angle)
{
	Play(TEXT("ChainStep"), Step.ChainDepth);
}

void UDIAudio::OnChainSettled(int32 Depth)
{
	if (Depth >= UDIGameSettings::Get().ChainSettleFrom)
	{
		Play(TEXT("ChainSettle"));
	}
}

void UDIAudio::Play(FName Cue, int32 Level)
{
	const FDISoundCue* Data = UDIGameSettings::Get().Cues.Find(Cue);
	if (!Data)
	{
		return;
	}
	for (const FDICueNote& Note : Data->Notes)
	{
		PlayNote(Note.Step + Level, Note.Volume, Note.Delay);
	}
	const int32 Haptic = Data->HapticMs + Data->HapticPerLevel * FMath::Min(Level, Data->HapticLevelCap);
	if (bHaptics && Haptic > 0)
	{
		DIPlatform::Vibrate(Haptic);
	}
}

void UDIAudio::PlayNote(int32 Step, float Volume, float Delay)
{
	if (!bSound)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (Delay <= 0.f || !World)
	{
		PlayNow(Step, Volume);
		return;
	}
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Step, Volume]()
	{
		PlayNow(Step, Volume);
	}), Delay, false);
}

void UDIAudio::PlayNow(int32 Step, float Volume)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<uint8>* Pcm = PcmCache.Find(Step);
	if (!Pcm)
	{
		const TArray<int16> Samples = DISynth::Pluck(DISynth::PentatonicFrequency(Step), DISynth::NoteSeconds, 0.6f);
		Pcm = &PcmCache.Add(Step);
		Pcm->Append(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(DISynth::SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = DISynth::NoteSeconds;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm->GetData(), Pcm->Num());

	UAudioComponent* Component = UGameplayStatics::SpawnSound2D(World, Wave, Volume, 1.f, 0.f, nullptr, false, true);
	if (Component)
	{
		// Procedural waves never end on their own; stop once the note has decayed.
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Component, [Component]()
		{
			Component->Stop();
		}), DISynth::NoteSeconds + 0.05f, false);
	}
}
