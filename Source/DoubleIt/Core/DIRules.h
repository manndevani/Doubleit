#pragma once

#include "DITypes.h"

/** Fusion math. Spec: docs/GAME_DESIGN.md#fusion */
namespace DIRules
{
	/** Plus between two equal numbers A. */
	DOUBLEIT_API int64 FirstFusion(EDIRuleSet Rules, int64 A);

	/** Equal outer pair O wrapping an already-fused center. */
	DOUBLEIT_API int64 ChainFusion(EDIRuleSet Rules, int64 Center, int64 Outer);
}
