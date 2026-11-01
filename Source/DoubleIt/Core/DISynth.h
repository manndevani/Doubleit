#pragma once

#include "CoreMinimal.h"

/** Tiny kalimba-like synth: every game sound is generated, no audio assets. Spec: docs/ART_AND_AUDIO.md#audio */
namespace DISynth
{
	constexpr int32 SampleRate = 44100;
	constexpr float NoteSeconds = 1.2f;

	/** Frequency of a C-major-pentatonic step relative to C5 (step 0). Negative steps go down. */
	DOUBLEIT_API float PentatonicFrequency(int32 Step);

	/** Mono 16-bit PCM: soft attack, exponential decay, a few inharmonic partials. */
	DOUBLEIT_API TArray<int16> Pluck(float Frequency, float Seconds = NoteSeconds, float Volume = 0.5f);
}
