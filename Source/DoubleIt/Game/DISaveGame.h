#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/DIMode.h"
#include "DISaveGame.generated.h"

UENUM()
enum class EDIThemeMode : uint8
{
	System,
	Light,
	Dark
};

/** Saved form of FDIPlayerProfile (Core stays free of UObject types). */
USTRUCT()
struct FDIProfileRecord
{
	GENERATED_BODY()

	UPROPERTY() int32 Runs = 0;
	UPROPERTY() float AvgMoves = 0.f;
	UPROPERTY() float AvgBestChain = 0.f;
	UPROPERTY() float PlusYield = 0.f;
	UPROPERTY() float AvgThinkSeconds = 0.f;
	UPROPERTY() int64 HighestEver = 0;

	FDIPlayerProfile ToProfile() const { return { Runs, AvgMoves, AvgBestChain, PlusYield, AvgThinkSeconds, HighestEver }; }
	static FDIProfileRecord From(const FDIPlayerProfile& P)
	{
		FDIProfileRecord R;
		R.Runs = P.Runs; R.AvgMoves = P.AvgMoves; R.AvgBestChain = P.AvgBestChain;
		R.PlusYield = P.PlusYield; R.AvgThinkSeconds = P.AvgThinkSeconds; R.HighestEver = P.HighestEver;
		return R;
	}
};

UCLASS()
class DOUBLEIT_API UDISaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("DoubleIt");

	/** Bump when the layout changes, and add the matching step in DISaveGame.cpp. */
	static constexpr int32 CurrentVersion = 1;

	/**
	 * Version this data was written with. Defaults to 0 on purpose: a save from before versioning has no such field,
	 * so it loads as 0 and gets every migration. New saves are stamped by MarkNew().
	 */
	UPROPERTY()
	int32 SaveVersion = 0;

	/** Runs the migration steps from SaveVersion up to CurrentVersion, in order. Returns true if anything ran. */
	bool Migrate();

	/** A save created fresh needs no migrations. */
	void MarkNew() { SaveVersion = CurrentVersion; }

	/** Best score per mode key ("Classic", "Addition", ...). */
	UPROPERTY()
	TMap<FName, int64> Bests;

	/** Version 0 fields: moved into Bests by the 0 -> 1 step, then zeroed. Kept so old saves still load. */
	UPROPERTY()
	int64 BestDoubling = 0;

	UPROPERTY()
	int64 BestAddition = 0;

	UPROPERTY()
	bool bSound = true;

	UPROPERTY()
	bool bHaptics = true;

	UPROPERTY()
	EDIThemeMode ThemeMode = EDIThemeMode::System;

	UPROPERTY()
	bool bTutorialDone = false;

	/** How this player plays each mode (key: mode id, e.g. "Classic"); drives the invisible adaptive pace. */
	UPROPERTY()
	TMap<FName, FDIProfileRecord> Profiles;
};
