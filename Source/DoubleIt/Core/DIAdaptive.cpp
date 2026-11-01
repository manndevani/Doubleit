#include "DIAdaptive.h"

namespace
{
	float Blend(float Average, float Sample, int32 Runs, float Rate)
	{
		return Runs == 0 ? Sample : FMath::Lerp(Average, Sample, Rate);
	}

	float Band(float Value, float Low, float High)
	{
		return FMath::Clamp((Value - Low) / FMath::Max(High - Low, 1e-3f), 0.f, 1.f);
	}
}

void DIAdaptive::Learn(FDIPlayerProfile& Profile, const FDIRunSummary& Run, const FDIAdaptiveTuning& A)
{
	const float Yield = Run.PlusesPlaced > 0 ? float(Run.Fusions) / Run.PlusesPlaced : Profile.PlusYield;
	Profile.AvgMoves = Blend(Profile.AvgMoves, float(Run.Moves), Profile.Runs, A.LearnRate);
	Profile.AvgBestChain = Blend(Profile.AvgBestChain, float(Run.BestChain), Profile.Runs, A.LearnRate);
	Profile.PlusYield = Blend(Profile.PlusYield, Yield, Profile.Runs, A.LearnRate);
	Profile.AvgThinkSeconds = Blend(Profile.AvgThinkSeconds, Run.AvgThinkSeconds, Profile.Runs, A.LearnRate);
	Profile.HighestEver = FMath::Max(Profile.HighestEver, Run.Highest);
	++Profile.Runs;
}

float DIAdaptive::Skill(const FDIPlayerProfile& Profile, const FDIAdaptiveTuning& A)
{
	// Survival matters most; long chains and well-used pluses show understanding of the ring.
	const float Survival = Band(Profile.AvgMoves, A.SurvivalLow, A.SurvivalHigh);
	const float Chains = Band(Profile.AvgBestChain, A.ChainLow, A.ChainHigh);
	const float Yield = FMath::Clamp(Profile.PlusYield, 0.f, 1.f);
	const float Raw = A.SurvivalWeight * Survival + A.ChainWeight * Chains + A.YieldWeight * Yield;

	const float Confidence = float(Profile.Runs) / (Profile.Runs + A.PriorRuns);
	return FMath::Lerp(0.5f, Raw, Confidence);
}

FDISpawnTuning DIAdaptive::TuningFor(const FDIPlayerProfile& Profile, const FDISpawnTuning& Base, const FDIAdaptiveTuning& A)
{
	FDISpawnTuning Tuning = Base;
	const float Lean = Skill(Profile, A) - 0.5f; // -0.5 .. 0.5
	if (Lean < 0.f)
	{
		const float Help = -Lean * 2.f; // 0 .. 1
		Tuning.PlusChance = Base.PlusChance + A.HelpPlusChance * Help;
		Tuning.MovesPerRangeStep = Base.MovesPerRangeStep + FMath::RoundToInt32(A.HelpGrowthMoves * Help);
		Tuning.PlusGuaranteeWindow = Help > A.HelpWindowFrom ? Base.PlusGuaranteeWindow - 1 : Base.PlusGuaranteeWindow;
	}
	else
	{
		const float Push = Lean * 2.f;
		Tuning.PlusChance = Base.PlusChance - A.PushPlusChance * Push;
		Tuning.MovesPerRangeStep = Base.MovesPerRangeStep - FMath::RoundToInt32(A.PushGrowthMoves * Push);
	}
	Tuning.PlusChance = FMath::Clamp(Tuning.PlusChance, A.MinPlusChance, A.MaxPlusChance);
	Tuning.MovesPerRangeStep = FMath::Clamp(Tuning.MovesPerRangeStep, A.MinGrowthMoves, A.MaxGrowthMoves);
	return Tuning;
}
