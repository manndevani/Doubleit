#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DIAudio.generated.h"

class UAudioComponent;
class UDIGameEvents;
struct FDIChord;
struct FDIPiece;
struct FDIFusionEvent;

/**
 * Plays synthesized notes (Core/DISynth), the sea ambience and haptics. Listens to UDIGameEvents and plays the
 * matching cue from UDIGameSettings::Cues on the tones of the current chord; nothing here knows the game rules.
 * Outer must be an actor in the game world.
 */
UCLASS()
class DOUBLEIT_API UDIAudio : public UObject
{
	GENERATED_BODY()

public:
	bool bHaptics = true;

	/** Subscribes to the game's events and starts the ambience. */
	void Listen(UDIGameEvents& Events);

	/** Master switch: notes and ambience. */
	void SetSoundOn(bool bOn);

	/** Slider levels, 0..1. */
	void SetVolumes(float Effects, float Ambience);

	/** Plays a cue by name at a level (chain depth, ring zone). Unknown names are ignored. */
	void Play(FName Cue, int32 Level = 0);

	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }

private:
	void OnPiecePlaced(const FDIPiece& Piece, float Angle);
	void OnFusionStep(const FDIFusionEvent& Step, float Angle);
	void OnChainSettled(int32 Depth);
	void OnRingFilled(float Fraction);
	void OnPauseChanged(bool bPaused);

	void PlayNote(int32 Step, float Volume, float Delay = 0.f);
	void PlayNow(float Frequency, float Volume);
	const FDIChord* CurrentChord() const;

	void StartAmbience();
	void StopAmbience();
	/** Fades every ambience layer to its level for the current sliders, fill and pause state. */
	void UpdateAmbience(float FadeSeconds);
	UAudioComponent* StartLoop(const TSharedRef<const TArray<uint8>>& Pcm);
	UAudioComponent* StartSea();
	class USoundWaveProcedural* NewStream();
	UAudioComponent* Spawn(class USoundWaveProcedural* Wave);

	bool bSound = true;
	float EffectsVolume = 1.f;
	float AmbienceVolume = 1.f;
	float Fill = 0.f;
	bool bPaused = false;
	int32 Moves = 0;
	FRandomStream Jitter; // its own stream: humanising never touches the game's seeded randomness

	TMap<int32, TArray<uint8>> PcmCache; // by frequency in centi-Hz
	TSharedPtr<const TArray<uint8>> CalmPcm;
	TSharedPtr<const TArray<uint8>> WarmPcm;

	UPROPERTY() TObjectPtr<UAudioComponent> Sea;
	UPROPERTY() TObjectPtr<UAudioComponent> CalmPad;
	UPROPERTY() TObjectPtr<UAudioComponent> WarmPad;
};
