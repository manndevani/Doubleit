#include "DISynth.h"

float DISynth::PentatonicFrequency(int32 Step)
{
	static const int32 Semitones[5] = { 0, 2, 4, 7, 9 };
	const int32 Octave = Step >= 0 ? Step / 5 : -((-Step + 4) / 5);
	const int32 Degree = Step - Octave * 5;
	const float C5 = 523.2511f;
	return C5 * FMath::Pow(2.f, Octave + Semitones[Degree] / 12.f);
}

TArray<int16> DISynth::Pluck(float Frequency, float Seconds, float Volume)
{
	const int32 Count = FMath::Max(1, FMath::RoundToInt(Seconds * SampleRate));
	TArray<int16> Samples;
	Samples.SetNumUninitialized(Count);

	const float Omega = UE_TWO_PI * Frequency;
	for (int32 i = 0; i < Count; ++i)
	{
		const float T = float(i) / SampleRate;
		const float Envelope = (1.f - FMath::Exp(-T / 0.004f)) * FMath::Exp(-T * 3.2f);
		const float Tone = FMath::Sin(Omega * T)
			+ 0.35f * FMath::Sin(2.f * Omega * T) * FMath::Exp(-T * 8.f)
			+ 0.12f * FMath::Sin(5.4f * Omega * T) * FMath::Exp(-T * 20.f);
		Samples[i] = int16(FMath::Clamp(Tone / 1.47f * Envelope * Volume, -1.f, 1.f) * 32767.f);
	}
	return Samples;
}
