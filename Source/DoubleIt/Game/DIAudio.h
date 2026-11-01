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

	void PlayPlace();
	void PlayFusion(int32 ChainDepth);
	void PlayGameOver();
	void PlayTutorialStep();

	virtual UWorld* GetWorld() const override { return GetOuter() ? GetOuter()->GetWorld() : nullptr; }

private:
	void PlayNote(int32 Step, float Volume, float Delay = 0.f);
	void PlayNow(int32 Step, float Volume);

	TMap<int32, TArray<uint8>> PcmCache;
};
