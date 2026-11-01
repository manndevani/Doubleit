#pragma once

#include "CoreMinimal.h"

/** Thin wrappers over OS features (Android via JNI + DoubleIt_UPL.xml, Windows for desktop testing). */
namespace DIPlatform
{
	bool IsSystemDarkMode();
	void Vibrate(int32 Milliseconds);
}
