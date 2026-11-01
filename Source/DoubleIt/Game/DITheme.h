#pragma once

#include "CoreMinimal.h"

/** UI colours per theme (Dusk Lagoon). Board colours live in Shaders/(name).hlsl. Spec: docs/ART_AND_AUDIO.md#palette */
struct FDITheme
{
	bool bDark = false;
	FLinearColor Ink;
	FLinearColor SoftInk;
	FLinearColor Panel;
	FLinearColor Button;
	FLinearColor ButtonPressed;
	FLinearColor Breeze; // the HUD breeze button, like the pearl breeze piece

	static FDITheme Get(bool bDark)
	{
		FDITheme T;
		T.bDark = bDark;
		if (bDark)
		{
			T.Ink = FLinearColor::FromSRGBColor(FColor(0xF2, 0xF4, 0xF8));
			T.SoftInk = FLinearColor::FromSRGBColor(FColor(0x8F, 0xB3, 0xC4));
			T.Panel = FLinearColor::FromSRGBColor(FColor(0x0C, 0x24, 0x33, 0xF0));
			T.Button = FLinearColor::FromSRGBColor(FColor(0x1E, 0x4A, 0x5C));
			T.ButtonPressed = FLinearColor::FromSRGBColor(FColor(0x2A, 0x60, 0x75));
			T.Breeze = FLinearColor::FromSRGBColor(FColor(0xF2, 0xF4, 0xF8));
		}
		else
		{
			T.Ink = FLinearColor::FromSRGBColor(FColor(0x10, 0x23, 0x3A));
			T.SoftInk = FLinearColor::FromSRGBColor(FColor(0x5A, 0x7A, 0x8C));
			T.Panel = FLinearColor::FromSRGBColor(FColor(0xE3, 0xF3, 0xF4, 0xF0));
			T.Button = FLinearColor::FromSRGBColor(FColor(0xCD, 0xE9, 0xEC));
			T.ButtonPressed = FLinearColor::FromSRGBColor(FColor(0xB5, 0xDD, 0xE2));
			T.Breeze = FLinearColor::FromSRGBColor(FColor(0xF2, 0xF4, 0xF8));
		}
		return T;
	}
};
