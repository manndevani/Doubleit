#include "DISynth.h"

namespace
{
	constexpr float C5 = 523.2511f;
	constexpr float CentsPerOctave = 1200.f;
	constexpr float Minus60Db = 6.9078f; // ln(1000)
	const TArray<int32> Pentatonic = { 0, 2, 4, 7, 9 };

	int16 ToPcm(float X)
	{
		return int16(FMath::Clamp(X, -1.f, 1.f) * 32767.f);
	}

	/**
	 * A small Schroeder room (four combs into two all-passes) per channel. Left and right use different delays,
	 * which is what makes it sound wide. Delays in samples at DISynth::SampleRate.
	 */
	struct FRoom
	{
		TArray<TArray<float>> Combs;
		TArray<int32> CombAt;
		TArray<float> CombGain;
		TArray<TArray<float>> Passes;
		TArray<int32> PassAt;

		FRoom(const TArray<int32>& CombDelays, const TArray<int32>& PassDelays, float Seconds)
		{
			for (int32 D : CombDelays)
			{
				Combs.Emplace_GetRef().SetNumZeroed(D);
				CombAt.Add(0);
				CombGain.Add(FMath::Exp(-Minus60Db * D / (Seconds * DISynth::SampleRate)));
			}
			for (int32 D : PassDelays)
			{
				Passes.Emplace_GetRef().SetNumZeroed(D);
				PassAt.Add(0);
			}
		}

		float Process(float In)
		{
			constexpr float PassGain = 0.5f;
			float Out = 0.f;
			for (int32 c = 0; c < Combs.Num(); ++c)
			{
				float& Slot = Combs[c][CombAt[c]];
				const float Delayed = Slot;
				Slot = In + Delayed * CombGain[c];
				CombAt[c] = (CombAt[c] + 1) % Combs[c].Num();
				Out += Delayed;
			}
			Out /= Combs.Num();
			for (int32 p = 0; p < Passes.Num(); ++p)
			{
				float& Slot = Passes[p][PassAt[p]];
				const float Delayed = Slot;
				Slot = Out + Delayed * PassGain;
				PassAt[p] = (PassAt[p] + 1) % Passes[p].Num();
				Out = Delayed - Out * PassGain;
			}
			return Out;
		}
	};
}

float DISynth::ScaleFrequency(int32 Step, const TArray<int32>& Tones, float Root)
{
	if (Tones.Num() == 0)
	{
		return Root;
	}
	const int32 N = Tones.Num();
	const int32 Octave = Step >= 0 ? Step / N : -((-Step + N - 1) / N);
	const int32 Degree = Step - Octave * N;
	return Root * FMath::Pow(2.f, Octave + Tones[Degree] / 12.f);
}

float DISynth::PentatonicFrequency(int32 Step)
{
	return ScaleFrequency(Step, Pentatonic, C5);
}

TArray<int16> DISynth::Note(const FVoice& Voice, float Frequency, float Volume)
{
	const int32 Count = FMath::Max(1, FMath::RoundToInt32(Voice.Seconds * SampleRate));
	TArray<int16> Out;
	Out.SetNumUninitialized(Count * 2);

	// Prime-ish delays (~25-40 ms combs, ~5 and ~1.7 ms all-passes), offset between channels.
	FRoom Left({ 1116, 1188, 1277, 1356 }, { 225, 556 }, Voice.RoomSeconds);
	FRoom Right({ 1139, 1211, 1300, 1379 }, { 248, 579 }, Voice.RoomSeconds);

	float GainSum = 0.f;
	for (const FPartial& P : Voice.Partials)
	{
		GainSum += P.Gain;
	}
	const float Norm = 1.f / FMath::Max(GainSum, 1e-3f);
	const float Spread = FMath::Pow(2.f, Voice.DetuneCents / CentsPerOctave);
	const float OmegaL = UE_TWO_PI * Frequency / Spread;
	const float OmegaR = UE_TWO_PI * Frequency * Spread;
	const float Dry = 1.f - Voice.RoomMix;

	for (int32 i = 0; i < Count; ++i)
	{
		const float T = float(i) / SampleRate;
		const float Attack = 1.f - FMath::Exp(-T / Voice.AttackSeconds);
		float L = 0.f;
		float R = 0.f;
		for (const FPartial& P : Voice.Partials)
		{
			const float Env = P.Gain * FMath::Exp(-T * P.Decay);
			L += Env * FMath::Sin(OmegaL * P.Ratio * T);
			R += Env * FMath::Sin(OmegaR * P.Ratio * T);
		}
		L *= Attack * Norm * Volume;
		R *= Attack * Norm * Volume;
		const float Fade = FMath::Min(1.f, (Count - i) / (0.05f * SampleRate)); // no click at the cut
		Out[i * 2] = ToPcm((Dry * L + Voice.RoomMix * Left.Process(L)) * Fade);
		Out[i * 2 + 1] = ToPcm((Dry * R + Voice.RoomMix * Right.Process(R)) * Fade);
	}
	return Out;
}

TArray<int16> DISynth::Pad(const FAmbience& A, const TArray<float>& Hz, float Level)
{
	const int32 Loop = A.LoopSeconds * AmbienceRate;
	const float Swells = FMath::Max(1.f, FMath::RoundToFloat(A.LoopSeconds / A.PadDriftSeconds));
	TArray<int16> Out;
	Out.SetNumUninitialized(Loop * 2);
	for (int32 i = 0; i < Loop; ++i)
	{
		const float T = float(i) / AmbienceRate;
		float L = 0.f, R = 0.f;
		for (int32 v = 0; v < Hz.Num(); ++v)
		{
			// Whole cycles per loop keep the seam silent; each voice swells on its own phase.
			const float F = FMath::RoundToFloat(Hz[v] * A.LoopSeconds) / A.LoopSeconds;
			const float Swell = 0.6f + 0.4f * FMath::Sin(UE_TWO_PI * Swells * float(i) / Loop + v * 2.1f);
			const float Pan = Hz.Num() > 1 ? float(v) / (Hz.Num() - 1) : 0.5f;
			const float S = FMath::Sin(UE_TWO_PI * F * T) * Swell;
			L += S * (1.f - 0.5f * Pan);
			R += S * (0.5f + 0.5f * Pan);
		}
		const float Norm = Level / FMath::Max(1, Hz.Num());
		Out[i * 2] = ToPcm(L * Norm);
		Out[i * 2 + 1] = ToPcm(R * Norm);
	}
	return Out;
}
