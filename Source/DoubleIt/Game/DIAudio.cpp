#include "DIAudio.h"
#include "DIPlatform.h"
#include "Core/DISynth.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

void UDIAudio::PlayPlaceAt(float Angle)
{
	// Eight low pentatonic notes around the ring: clockwise from the top climbs gently.
	const float Turn = FMath::Fmod(Angle + 4.f * UE_TWO_PI, UE_TWO_PI) / UE_TWO_PI;
	PlayNote(-6 + FMath::Min(FMath::FloorToInt32(Turn * 8.f), 7), 0.2f);
	if (bHaptics)
	{
		DIPlatform::Vibrate(8);
	}
}

void UDIAudio::PlayChainStep(int32 ChainDepth)
{
	PlayNote(ChainDepth + 1, 0.34f);
	if (bHaptics)
	{
		DIPlatform::Vibrate(14 + 6 * FMath::Min(ChainDepth, 5));
	}
}

void UDIAudio::PlayChainSettle()
{
	PlayNote(-3, 0.14f, 0.05f);
}

void UDIAudio::PlayBreeze()
{
	for (int32 i = 0; i < 5; ++i)
	{
		PlayNote(7 - 2 * i, 0.16f, i * 0.06f);
	}
	if (bHaptics)
	{
		DIPlatform::Vibrate(16);
	}
}

void UDIAudio::PlayReveal(float Delay)
{
	PlayNote(5, 0.26f, Delay);
	PlayNote(7, 0.26f, Delay + 0.09f);
	PlayNote(9, 0.3f, Delay + 0.18f);
}

void UDIAudio::PlayNewBest()
{
	PlayNote(2, 0.18f);
	PlayNote(4, 0.18f, 0.03f);
	PlayNote(7, 0.2f, 0.06f);
	if (bHaptics)
	{
		DIPlatform::Vibrate(24);
	}
}

void UDIAudio::PlayLift()
{
	PlayNote(3, 0.18f);
	PlayNote(1, 0.16f, 0.07f);
	if (bHaptics)
	{
		DIPlatform::Vibrate(8);
	}
}

void UDIAudio::PlayGameOver()
{
	PlayNote(4, 0.25f, 0.f);
	PlayNote(2, 0.25f, 0.3f);
	PlayNote(0, 0.25f, 0.6f);
	if (bHaptics)
	{
		DIPlatform::Vibrate(40);
	}
}

void UDIAudio::PlayTutorialStep()
{
	PlayNote(2, 0.3f, 0.f);
	PlayNote(4, 0.3f, 0.1f);
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
