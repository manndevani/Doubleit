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

namespace
{
	constexpr float NoteLevel = 0.6f;     // synth headroom; cue volumes scale from here
	constexpr float StopMargin = 0.05f;   // s after a note's tail before its component stops
	constexpr int32 SeaSeed = 7;

	TSharedRef<const TArray<uint8>> ToBytes(const TArray<int16>& Samples)
	{
		TSharedRef<TArray<uint8>> Bytes = MakeShared<TArray<uint8>>();
		Bytes->Append(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
		return Bytes;
	}

	const UDIGameSettings& Settings() { return UDIGameSettings::Get(); }
}

void UDIAudio::Listen(UDIGameEvents& Events)
{
	Events.PiecePlaced.AddUObject(this, &UDIAudio::OnPiecePlaced);
	Events.FusionStep.AddUObject(this, &UDIAudio::OnFusionStep);
	Events.ChainSettled.AddUObject(this, &UDIAudio::OnChainSettled);
	Events.RingFilled.AddUObject(this, &UDIAudio::OnRingFilled);
	Events.PauseChanged.AddUObject(this, &UDIAudio::OnPauseChanged);
	Events.PieceLifted.AddUObject(this, &UDIAudio::Play, FName(TEXT("Lift")), 0);
	Events.NumberRevealed.AddUObject(this, &UDIAudio::Play, FName(TEXT("Reveal")), 0);
	Events.NewBest.AddUObject(this, &UDIAudio::Play, FName(TEXT("NewBest")), 0);
	Events.GameOver.AddUObject(this, &UDIAudio::Play, FName(TEXT("GameOver")), 0);
	Events.TutorialAdvanced.AddUObject(this, &UDIAudio::Play, FName(TEXT("TutorialStep")), 0);
	if (bSound)
	{
		StartAmbience();
	}
}

void UDIAudio::SetSoundOn(bool bOn)
{
	if (bSound == bOn)
	{
		return;
	}
	bSound = bOn;
	if (bSound)
	{
		StartAmbience();
	}
	else
	{
		StopAmbience();
	}
}

void UDIAudio::SetVolumes(float Effects, float Ambience)
{
	EffectsVolume = FMath::Clamp(Effects, 0.f, 1.f);
	AmbienceVolume = FMath::Clamp(Ambience, 0.f, 1.f);
	UpdateAmbience(0.f);
}

void UDIAudio::OnPiecePlaced(const FDIPiece& Piece, float Angle)
{
	++Moves;
	const FName Cue = DIPieces::Of(Piece).LandCue();
	const FDISoundCue* Data = Settings().Cues.Find(Cue);
	int32 Level = 0;
	if (Data && Data->bLevelFromAngle)
	{
		const int32 Zones = FMath::Max(Settings().PlaceZones, 1);
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
	if (Depth >= Settings().ChainSettleFrom)
	{
		Play(TEXT("ChainSettle"));
	}
}

void UDIAudio::OnRingFilled(float Fraction)
{
	Fill = FMath::Clamp(Fraction, 0.f, 1.f);
	UpdateAmbience(Settings().AmbienceFadeSeconds);
}

void UDIAudio::OnPauseChanged(bool bInPaused)
{
	bPaused = bInPaused;
	UpdateAmbience(Settings().AmbienceFadeSeconds);
}

void UDIAudio::Play(FName Cue, int32 Level)
{
	const FDISoundCue* Data = Settings().Cues.Find(Cue);
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

const FDIChord* UDIAudio::CurrentChord() const
{
	const TArray<FDIChord>& Chords = Settings().Progression;
	if (Chords.Num() == 0)
	{
		return nullptr;
	}
	return &Chords[(Moves / FMath::Max(Settings().MovesPerChord, 1)) % Chords.Num()];
}

void UDIAudio::PlayNote(int32 Step, float Volume, float Delay)
{
	if (!bSound || EffectsVolume <= 0.f)
	{
		return;
	}
	// The chord is read when the cue starts, so an arpeggio never straddles a chord change.
	const FDIChord* Chord = CurrentChord();
	const float Frequency = Chord ? DISynth::ScaleFrequency(Step, Chord->Tones, Settings().HarmonyRootHz) : DISynth::PentatonicFrequency(Step);
	const float Human = Volume * (1.f + Jitter.FRandRange(-Settings().VolumeJitter, Settings().VolumeJitter));
	UWorld* World = GetWorld();
	if (Delay <= 0.f || !World)
	{
		PlayNow(Frequency, Human); // the first note answers the tap at once
		return;
	}
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Frequency, Human]()
	{
		PlayNow(Frequency, Human);
	}), Delay + Jitter.FRandRange(0.f, Settings().TimingJitter), false);
}

void UDIAudio::PlayNow(float Frequency, float Volume)
{
	UWorld* World = GetWorld();
	if (!World || !bSound)
	{
		return;
	}

	const DISynth::FVoice Voice;
	const int32 Key = FMath::RoundToInt32(Frequency * 100.f);
	TArray<uint8>* Pcm = PcmCache.Find(Key);
	if (!Pcm)
	{
		const TArray<int16> Samples = DISynth::Note(Voice, Frequency, NoteLevel);
		Pcm = &PcmCache.Add(Key);
		Pcm->Append(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(DISynth::SampleRate);
	Wave->NumChannels = 2;
	Wave->Duration = Voice.Seconds;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm->GetData(), Pcm->Num());

	UAudioComponent* Component = UGameplayStatics::SpawnSound2D(World, Wave, Volume * EffectsVolume, 1.f, 0.f, nullptr, false, true);
	if (Component)
	{
		// Procedural waves never end on their own; stop once the note has decayed.
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Component, [Component]()
		{
			Component->Stop();
		}), Voice.Seconds + StopMargin, false);
	}
}

void UDIAudio::StartAmbience()
{
	if (!GetWorld())
	{
		return;
	}
	const DISynth::FAmbience Ambience;
	if (!SeaPcm)
	{
		// Generated once (a few tens of ms) and kept: three loops of ~2 MB each.
		SeaPcm = ToBytes(DISynth::Sea(Ambience, SeaSeed));
		CalmPcm = ToBytes(DISynth::Pad(Ambience, Ambience.PadHz, Ambience.PadLevel));
		WarmPcm = ToBytes(DISynth::Pad(Ambience, Settings().WarmPadHz, Ambience.PadLevel));
	}
	StopAmbience();
	Sea = StartLoop(SeaPcm.ToSharedRef());
	CalmPad = StartLoop(CalmPcm.ToSharedRef());
	WarmPad = StartLoop(WarmPcm.ToSharedRef());
	UpdateAmbience(Settings().AmbienceFadeSeconds);
}

void UDIAudio::StopAmbience()
{
	for (UAudioComponent* Loop : { Sea.Get(), CalmPad.Get(), WarmPad.Get() })
	{
		if (Loop)
		{
			Loop->Stop();
			Loop->DestroyComponent();
		}
	}
	Sea = CalmPad = WarmPad = nullptr;
}

void UDIAudio::UpdateAmbience(float FadeSeconds)
{
	const float Level = AmbienceVolume * (bPaused ? Settings().PausedAmbience : 1.f);
	auto Fade = [FadeSeconds](UAudioComponent* Loop, float Target)
	{
		if (Loop)
		{
			Loop->AdjustVolume(FadeSeconds, FMath::Max(Target, 0.f));
		}
	};
	Fade(Sea, Settings().SeaGain * Level);
	Fade(CalmPad, Settings().CalmPadGain * Level);
	Fade(WarmPad, Settings().WarmPadGain * Fill * Level);
}

UAudioComponent* UDIAudio::StartLoop(const TSharedRef<const TArray<uint8>>& Pcm)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(DISynth::AmbienceRate);
	Wave->NumChannels = 2;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	// Runs on the audio thread: queue the same loop again whenever it runs low. The buffer is shared and immutable.
	Wave->OnSoundWaveProceduralUnderflow.BindLambda([Pcm](USoundWaveProcedural* Procedural, int32 SamplesRequired)
	{
		Procedural->QueueAudio(Pcm->GetData(), Pcm->Num());
	});
	Wave->QueueAudio(Pcm->GetData(), Pcm->Num());
	// Starts silent; UpdateAmbience fades it in.
	UAudioComponent* Component = UGameplayStatics::SpawnSound2D(GetWorld(), Wave, 1.f, 1.f, 0.f, nullptr, false, false);
	if (Component)
	{
		Component->SetVolumeMultiplier(1.f);
		Component->AdjustVolume(0.f, 0.f);
	}
	return Component;
}
