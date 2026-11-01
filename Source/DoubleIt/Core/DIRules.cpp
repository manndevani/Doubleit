#include "DIRules.h"

int64 DIRules::FirstFusion(EDIRuleSet Rules, int64 A)
{
	return A + A;
}

int64 DIRules::ChainFusion(EDIRuleSet Rules, int64 Center, int64 Outer)
{
	switch (Rules)
	{
	case EDIRuleSet::Doubling:
		return Outer < Center ? Center * 2 : Outer * 4;
	case EDIRuleSet::Addition:
	default:
		return Center + Outer + Outer;
	}
}
