#include "DIMode.h"

namespace
{
	/** Classic: equal pairs double; chains climb the power-of-two ladder (`O < C ? 2C : 4O`). */
	class FDIDoublingRules final : public IDIRuleSet
	{
	public:
		virtual int64 FirstFusion(int64 Outer) const override { return Outer * 2; }
		virtual int64 ChainFusion(int64 Center, int64 Outer) const override { return Outer < Center ? Center * 2 : Outer * 4; }
		virtual int64 SpawnNumber(int32 Level, FRandomStream& Stream) const override
		{
			return int64(1) << FMath::Min(Level + Stream.RandRange(0, 2), 62);
		}
		virtual int32 ColourRank(int64 Value) const override { return FMath::FloorLog2_64(uint64(FMath::Max<int64>(Value, 1))); }
	};

	/** Addition: pairs add up; chains add the outer pair to the centre. */
	class FDIAdditionRules final : public IDIRuleSet
	{
	public:
		virtual int64 FirstFusion(int64 Outer) const override { return Outer * 2; }
		virtual int64 ChainFusion(int64 Center, int64 Outer) const override { return Center + Outer * 2; }
		virtual int64 SpawnNumber(int32 Level, FRandomStream& Stream) const override
		{
			// Merges leave the spawn range quickly, so only two adjacent values spawn to keep pairs common.
			return 1 + Level + Stream.RandRange(0, 1);
		}
		virtual int32 ColourRank(int64 Value) const override { return int32(FMath::Min<int64>(Value, MAX_int32)); }
	};

	TArray<FDIMode>& Registry()
	{
		static TArray<FDIMode> Modes = []
		{
			TArray<FDIMode> Built;
			Built.Add({ DIModes::Classic, MakeShared<FDIDoublingRules>(), FDIModeConfig() });

			FDIModeConfig Addition;
			Addition.RevealFrom = 6;
			Addition.RewardFrom = 9; // as rare as 256 in Classic (the ninth rank)
			Built.Add({ DIModes::Addition, MakeShared<FDIAdditionRules>(), Addition });
			return Built;
		}();
		return Modes;
	}
}

void DIModes::Register(const FDIMode& Mode)
{
	check(Mode.IsValid());
	if (FDIMode* Existing = Registry().FindByPredicate([&](const FDIMode& M) { return M.Key == Mode.Key; }))
	{
		*Existing = Mode;
		return;
	}
	Registry().Add(Mode);
}

bool DIModes::Contains(FName Key)
{
	return Registry().ContainsByPredicate([&](const FDIMode& M) { return M.Key == Key; });
}

FDIMode DIModes::Get(FName Key)
{
	const FDIMode* Found = Registry().FindByPredicate([&](const FDIMode& M) { return M.Key == Key; });
	return Found ? *Found : Registry()[0];
}

TArray<FName> DIModes::Keys()
{
	TArray<FName> Out;
	for (const FDIMode& M : Registry())
	{
		Out.Add(M.Key);
	}
	return Out;
}
