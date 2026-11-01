#pragma once

#include "CoreMinimal.h"

/**
 * Tiny synth: every game sound is generated, no audio assets. Pure maths, no engine audio.
 * Spec: docs/ART_AND_AUDIO.md#audio
 */
namespace DISynth
{
	constexpr int32 SampleRate = 32000;   // notes have little above 8 kHz; saves memory over 44.1k
	constexpr int32 AmbienceRate = 22050; // the bed is all low, soft content

	/** One overtone of a voice: frequency ratio to the fundamental, level, and how fast it dies (1/s). */
	struct FPartial
	{
		float Ratio = 1.f;
		float Gain = 1.f;
		float Decay = 3.f;
	};

	/** A struck-glass / soft-marimba voice with a baked stereo room. All levels 0..1. */
	struct FVoice
	{
		TArray<FPartial> Partials = { { 1.f, 1.f, 2.4f }, { 2.f, 0.18f, 5.f }, { 3.93f, 0.1f, 9.f }, { 6.8f, 0.035f, 16.f } };
		float AttackSeconds = 0.008f;   // soft mallet, no click
		float Seconds = 1.9f;           // note length including the room tail
		float DetuneCents = 3.f;        // left and right a hair apart: width without chorus wobble
		float RoomMix = 0.22f;          // share of the room in the mix
		float RoomSeconds = 1.4f;       // time for the room to fall by 60 dB
	};

	/**
	 * Frequency of a step on a scale given as semitones within one octave above Root (sorted, e.g. a chord's tones).
	 * Step 0 is the first tone; each Tones.Num() steps is one octave; negative steps go down.
	 */
	DOUBLEIT_API float ScaleFrequency(int32 Step, const TArray<int32>& Tones, float Root);

	/** C-major pentatonic from C5: the scale used when no harmony is set. */
	DOUBLEIT_API float PentatonicFrequency(int32 Step);

	/** Interleaved stereo 16-bit PCM at SampleRate. */
	DOUBLEIT_API TArray<int16> Note(const FVoice& Voice, float Frequency, float Volume);

	/** A quiet pad, generated once as a loop of whole seconds so it is seamless. The sea is live: Core/DISea. */
	struct FAmbience
	{
		int32 LoopSeconds = 24;
		TArray<float> PadHz = { 130.81f, 196.f, 261.63f }; // C3 G3 C4: an open fifth that sits under every chord
		float PadLevel = 0.18f;
		float PadDriftSeconds = 12.f;     // the pad's slow swell
	};

	/** Interleaved stereo 16-bit PCM at AmbienceRate: a pad of sines at Hz with a slow swell. */
	DOUBLEIT_API TArray<int16> Pad(const FAmbience& Ambience, const TArray<float>& Hz, float Level);
}
