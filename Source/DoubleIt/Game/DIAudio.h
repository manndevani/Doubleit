#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DIAudio.generated.h"

class UDIGameEvents;
struct FDIPiece;
struct FDIFusionEvent;

/**
 * Plays synthesized notes (Core/DISynth) and haptics. Listens to UDIGameEvents and plays the matching cue from
 * UDIGameSettings::Cues; nothing here knows the game rules. Outer must be an actor in the game world.
 */
UCLASS()
class DOUBLEIT_API UDIAudio : public UObject
{
	GENERATED_BODY()

public:
	bool bSound = true;
	bool bHaptics = true;

	/** Subscribes to the game's events. */
	void Listen(UDIGameEvents& Events);

	/** Plays a cue by name at a level (chain depth, ring zone). Unknown names are ignored. */
	void Play(FName Cue, int32 Level = 0);

	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }

private:
	void OnPiecePlaced(const FDIPiece& Piece, float Angle);
	void OnFusionStep(const FDIFusionEvent& Step, float Angle);
	void OnChainSettled(int32 Depth);

	void PlayNote(int32 Step, float Volume, float Delay = 0.f);
	void PlayNow(int32 Step, float Volume);

	TMap<int32, TArray<uint8>> PcmCache;
};
