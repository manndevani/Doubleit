#include "DISea.h"
#include "DISynth.h"

namespace
{
	constexpr float FoamFromHz = 1500.f;  // foam is what is left above this
	constexpr float WaveGoneFalls = 10.f; // a wave is dropped after its rise + this many fall time constants (inaudible by then)
	constexpr float FirstWaveSeconds = 0.8f;

	/** One-pole low-pass coefficient for a cutoff. */
	float Coef(float Hz, int32 Rate)
	{
		return 1.f - FMath::Exp(-UE_TWO_PI * Hz / Rate);
	}

	/** tanh-like curve (rational, cheap): unity near 0, rounds off to +-1. */
	float SoftLimit(float X)
	{
		X = FMath::Clamp(X, -3.f, 3.f);
		return X * (27.f + X * X) / (27.f + 9.f * X * X);
	}

}

FDISea::FDISea(int32 SampleRate, int32 Seed, const FSettings& Settings)
	: S(Settings)
	, Rate(FMath::Max(SampleRate, 1))
	, State(uint32(Seed) * 2654435761u + 0x9E3779B9u)
{
	if (State == 0)
	{
		State = 1;
	}
	NextWaveAt = FirstWaveSeconds;
}

float FDISea::Noise()
{
	State ^= State << 13;
	State ^= State >> 17;
	State ^= State << 5;
	return float(State) * (2.f / 4294967296.f) - 1.f;
}

void FDISea::ScheduleNext()
{
	FWave& W = Waves.AddDefaulted_GetRef();
	W.Start = Now;
	W.Rise = S.RiseMin + S.RiseSpread * Uniform();
	W.Fall = S.FallMin + S.FallSpread * Uniform();
	W.Strength = FMath::Lerp(S.StrengthMin, 1.f, Uniform());
	W.Pan = S.PanSpread * Noise();
	NextWaveAt = Now + S.WaveEveryMin + S.WaveEverySpread * Uniform();
}

void FDISea::Render(TArray<int16>& Out, int32 Frames)
{
	const float Dt = 1.f / Rate;
	const float FarCoef = Coef(S.DistantHz, Rate);
	const float HighCoef = Coef(FoamFromHz, Rate);
	const float FlutterCoef = Coef(S.FoamFlutterHz, Rate);
	Out.Reserve(Out.Num() + Frames * 2);

	for (int32 f = 0; f < Frames; ++f)
	{
		if (Now >= NextWaveAt)
		{
			ScheduleNext();
		}

		// Sum the waves: swell (eased in), then an exponential wash back. Foam lives in the falling part.
		float Swell[2] = {}, Break[2] = {}, Foam[2] = {};
		for (const FWave& W : Waves)
		{
			const float T = float(Now - W.Start);
			const float Rise = FMath::Min(T / W.Rise, 1.f);
			const float Env = T < W.Rise ? Rise * Rise : FMath::Exp(-(T - W.Rise) / W.Fall);
			const float After = T < W.Rise ? 0.f : Env;
			const float Right = 0.5f + 0.5f * W.Pan;
			const float Side[2] = { FMath::Sqrt(1.f - Right), FMath::Sqrt(Right) };
			for (int32 c = 0; c < 2; ++c)
			{
				Swell[c] += W.Strength * Env * Side[c];
				Break[c] += W.Strength * Env * Env * Side[c]; // brightest right at the break
				Foam[c] += W.Strength * After * (1.f - After) * 4.f * Side[c]; // peaks halfway down the wash
			}
		}

		Flutter += FlutterCoef * (FMath::Abs(Noise()) - Flutter);
		const float Shared = Noise(); // a little common noise keeps the stereo image solid, not two separate seas
		float Mix[2];
		for (int32 c = 0; c < 2; ++c)
		{
			const float N = 0.75f * Noise() + 0.25f * Shared;

			// Wave body: a 12 dB/oct low-pass that opens from rumble towards the crash as the wave breaks.
			const float Cut = FMath::Lerp(S.RumbleHz, S.CrashHz, FMath::Min(Break[c], 1.f));
			const float K = Coef(Cut, Rate);
			Body[c][0] += K * (N - Body[c][0]);
			Body[c][1] += K * (Body[c][0] - Body[c][1]);
			Hiss[c] += Coef(S.CrashHz, Rate) * (N - Hiss[c]);

			Far[c] += FarCoef * (N - Far[c]);
			HighRef[c] += HighCoef * (N - HighRef[c]);
			const float High = N - HighRef[c];

			Mix[c] = S.DistantLevel * Far[c] * 4.f
				+ S.RumbleLevel * Body[c][1] * 3.f * FMath::Min(Swell[c], 1.5f)
				+ S.HissLevel * Hiss[c] * FMath::Min(Break[c], 1.f)
				+ S.FoamLevel * High * FMath::Min(Foam[c], 1.f) * Flutter * 3.f;
		}
		// Soft limiter: overlapping waves round off instead of clipping.
		Out.Add(DISynth::ToPcm(SoftLimit(Mix[0] * S.Level)));
		Out.Add(DISynth::ToPcm(SoftLimit(Mix[1] * S.Level)));

		Now += Dt;
	}

	Waves.RemoveAll([this](const FWave& W) { return Now - W.Start > W.Rise + WaveGoneFalls * W.Fall; });
}
