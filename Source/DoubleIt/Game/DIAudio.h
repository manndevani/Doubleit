#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DIAudio.generated.h"

/** Plays synthesized notes (Core/DISynth) and haptics. Outer must be an actor in the game world. */
UCLASS()
class DOUBLEIT_API UDIAudio : public UObject
{
	GENERATED_BODY()

public:
	bool bSound = true;
	bool bHaptics = true;

	/** Soft note whose pitch follows where the piece landed (angle on the ring), so ordinary moves sound musical. */
	void PlayPlaceAt(float Angle);
	/** One collision of a chain: each step is a higher note and a firmer haptic tick. */
	void PlayChainStep(int32 ChainDepth);
	/** Low, quiet note that settles a chain of 2+. */
	void PlayChainSettle();
	/** Breeze: a quick high-to-low wind-chime cascade. */
	void PlayBreeze();
	/** A number made for the first time ever: a bright rising three-note chime. */
	void PlayReveal(float Delay);
	/** Passing your best score mid-run: a soft major chord. */
	void PlayNewBest();
	void PlayLift();
	void PlayGameOver();
	void PlayTutorialStep();

	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }

private:
	void PlayNote(int32 Step, float Volume, float Delay = 0.f);
	void PlayNow(int32 Step, float Volume);

	TMap<int32, TArray<uint8>> PcmCache;
};
