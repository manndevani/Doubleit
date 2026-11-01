#pragma once

#include "CoreMinimal.h"
#include "Internationalization/StringTableRegistry.h"

/**
 * Player-facing text lives in the string table Content/DoubleIt/Text/DoubleIt.csv (registered by the module at start-up,
 * gathered by the localization dashboard, staged as NonUFS). Never put player text in code; add a row and use DIText::Get.
 */
namespace DIText
{
	inline const TCHAR* TableId = TEXT("DoubleIt");

	inline FText Get(const FString& Key) { return FText::FromStringTable(TableId, Key); }

	/** Numbers as typed (no thousands separator), for scores. */
	inline FText Number(int64 Value) { return FText::AsNumber(Value, &FNumberFormattingOptions::DefaultNoGrouping()); }
}
