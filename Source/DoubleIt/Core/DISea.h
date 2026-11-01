#pragma once

#include "CoreMinimal.h"

/**
 * A sea that never loops: rendered live, a block at a time, from seeded randomness. Waves arrive at irregular times,
 * each with its own strength, length and side; a wave swells as a low rumble, breaks into a brighter hiss, then pulls
 * back with fizzing foam. A deep distant wash runs underneath. Pure maths: the audio layer streams Render() output.
 * Spec: docs/ART_AND_AUDIO.md#audio-and-haptics-all-generated-no-assets
 */
class DOUBLEIT_API FDISea
{
public:
	struct FSettings
	{
		float WaveEveryMin = 5.f;     // s between wave arrivals: Min + random Spread
		float WaveEverySpread = 7.f;
		float RiseMin = 1.6f;         // s a wave swells before it breaks
		float RiseSpread = 1.6f;
		float FallMin = 2.5f;         // s for the wash to fall back (time constant of the decay)
		float FallSpread = 2.5f;
		float StrengthMin = 0.45f;    // wave loudness 0..1
		float PanSpread = 0.7f;       // waves land anywhere from -PanSpread (left) to +PanSpread (right)
		float RumbleHz = 220.f;       // body of a wave
		float CrashHz = 2600.f;       // brightest hiss at the break
		float DistantHz = 140.f;      // the far-off wash under everything
		float DistantLevel = 0.22f;
		float RumbleLevel = 0.55f;
		float HissLevel = 0.32f;
		float FoamLevel = 0.12f;
		float FoamFlutterHz = 14.f;   // how fast the foam bubbles
		float Level = 0.9f;           // output before the soft limiter
	};

	FDISea(int32 SampleRate, int32 Seed, const FSettings& Settings);
	FDISea(int32 SampleRate, int32 Seed) : FDISea(SampleRate, Seed, FSettings()) {} // no default argument: Clang needs FSettings complete

	/** Appends Frames stereo frames (interleaved 16-bit) to Out. Consecutive calls continue seamlessly. */
	void Render(TArray<int16>& Out, int32 Frames);

	int32 GetSampleRate() const { return Rate; }

private:
	struct FWave
	{
		double Start = 0.0;
		float Rise = 0.f;
		float Fall = 0.f;
		float Strength = 0.f;
		float Pan = 0.f;
	};

	/** Uniform in [-1, 1). xorshift: cheap enough for the audio thread, deterministic per seed. */
	float Noise();
	float Uniform() { return 0.5f * (Noise() + 1.f); }
	void ScheduleNext();

	FSettings S;
	int32 Rate;
	uint32 State;
	double Now = 0.0;
	double NextWaveAt = 0.0;
	TArray<FWave> Waves;

	// Filter state per channel: two one-pole low-passes for the wave body, one for the hiss, one for the distant wash.
	float Body[2][2] = {};
	float Hiss[2] = {};
	float Far[2] = {};
	float HighRef[2] = {};
	float Flutter = 0.f;
};
