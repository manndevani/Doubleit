#include "DIAdaptive.h"

namespace
{
	float Blend(float Average, float Sample, int32 Runs)
	{
		return Runs == 0 ? Sample : FMath::Lerp(Average, Sample, DIAdaptive::LearnRate);
	}
}

void DIAdaptive::Learn(FDIPlayerProfile& Profile, const FDIRunSummary& Run)
{
	const float Yield = Run.PlusesPlaced > 0 ? float(Run.Fusions) / Run.PlusesPlaced : Profile.PlusYield;
	Profile.AvgMoves = Blend(Profile.AvgMoves, float(Run.Moves), Profile.Runs);
	Profile.AvgBestChain = Blend(Profile.AvgBestChain, float(Run.BestChain), Profile.Runs);
	Profile.PlusYield = Blend(Profile.PlusYield, Yield, Profile.Runs);
	Profile.AvgThinkSeconds = Blend(Profile.AvgThinkSeconds, Run.AvgThinkSeconds, Profile.Runs);
	Profile.HighestEver = FMath::Max(Profile.HighestEver, Run.Highest);
	++Profile.Runs;
}

float DIAdaptive::Skill(const FDIPlayerProfile& Profile)
{
	// Survival matters most; long chains and well-used pluses show understanding of the ring.
	const float Survival = FMath::Clamp((Profile.AvgMoves - 30.f) / 90.f, 0.f, 1.f);  // 30 moves .. 120 moves
	const float Chains = FMath::Clamp((Profile.AvgBestChain - 1.f) / 4.f, 0.f, 1.f); // 1 .. 5
	const float Yield = FMath::Clamp(Profile.PlusYield, 0.f, 1.f);
	const float Raw = 0.6f * Survival + 0.25f * Chains + 0.15f * Yield;

	// Trust grows with evidence: one run is a hint, five runs are a pattern.
	const float Confidence = float(Profile.Runs) / (Profile.Runs + 2.f);
	return FMath::Lerp(0.5f, Raw, Confidence);
}

FDISpawnTuning DIAdaptive::TuningFor(const FDIPlayerProfile& Profile, const FDISpawnTuning& Base)
{
	FDISpawnTuning Tuning = Base;
	const float Lean = Skill(Profile) - 0.5f; // -0.5 .. 0.5
	if (Lean < 0.f)
	{
		// Help is stronger than challenge: losing early is what makes people quit.
		const float Help = -Lean * 2.f; // 0 .. 1
		Tuning.PlusChance = Base.PlusChance + 0.05f * Help;
		Tuning.MovesPerRangeStep = Base.MovesPerRangeStep + FMath::RoundToInt32(10.f * Help);
		Tuning.PlusGuaranteeWindow = Help > 0.5f ? Base.PlusGuaranteeWindow - 1 : Base.PlusGuaranteeWindow;
	}
	else
	{
		const float Push = Lean * 2.f;
		Tuning.PlusChance = Base.PlusChance - 0.03f * Push;
		Tuning.MovesPerRangeStep = Base.MovesPerRangeStep - FMath::RoundToInt32(6.f * Push);
	}
	Tuning.PlusChance = FMath::Clamp(Tuning.PlusChance, 0.14f, 0.22f);
	Tuning.MovesPerRangeStep = FMath::Clamp(Tuning.MovesPerRangeStep, 34, 50);
	return Tuning;
}
