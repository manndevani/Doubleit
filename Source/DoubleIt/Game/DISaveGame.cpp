#include "DISaveGame.h"
#include "DoubleIt.h"
#include "DIGameSettings.h"

namespace
{
	/** 0 -> 1: best scores keyed by mode instead of one field per mode. */
	void BestsByMode(UDISaveGame& Save)
	{
		auto Move = [&Save](int64& Old, const TCHAR* Key)
		{
			if (Old > 0)
			{
				int64& Best = Save.Bests.FindOrAdd(Key);
				Best = FMath::Max(Best, Old);
				Old = 0;
			}
		};
		Move(Save.BestDoubling, DIModes::Classic);
		Move(Save.BestAddition, DIModes::Addition);
	}

	/** 1 -> 2: effects and ambience volume sliders, starting at the defaults. */
	void AudioLevels(UDISaveGame& Save)
	{
		Save.EffectsVolume = UDIGameSettings::Get().DefaultEffectsVolume;
		Save.AmbienceVolume = UDIGameSettings::Get().DefaultAmbienceVolume;
	}

	/** 2 -> 3: the run in progress is saved; older saves have none to resume. */
	void SavedRun(UDISaveGame& Save)
	{
		Save.RunData.Reset();
	}

	/** 3 -> 4: owned breezes. Everyone starts with the new-player gift; numbers already made never pay out again. */
	void OwnedPieces(UDISaveGame& Save)
	{
		Save.Owned = UDIGameSettings::Get().StartingPieces;
		for (const TPair<FName, FDIProfileRecord>& Mode : Save.Profiles)
		{
			Save.RewardedUpTo.Add(Mode.Key, Mode.Value.HighestEver);
		}
	}

	/** 4 -> 5: the optional banner; everyone starts with it off (opt-in). */
	void AdsOptIn(UDISaveGame& Save)
	{
		Save.bAdsOn = false;
	}

	/** Step i upgrades version i to i + 1. Only ever append; never edit a shipped step. */
	using FMigrationStep = void (*)(UDISaveGame&);
	constexpr FMigrationStep Steps[] = { &BestsByMode, &AudioLevels, &SavedRun, &OwnedPieces, &AdsOptIn };
	static_assert(UE_ARRAY_COUNT(Steps) == UDISaveGame::CurrentVersion, "one migration step per version");
}

bool UDISaveGame::Migrate()
{
	if (SaveVersion > CurrentVersion)
	{
		UE_LOG(LogDoubleIt, Warning, TEXT("Save version %d is newer than this build (%d); loading as is"), SaveVersion, CurrentVersion);
		return false;
	}
	const bool bRan = SaveVersion < CurrentVersion;
	for (; SaveVersion < CurrentVersion; ++SaveVersion)
	{
		Steps[SaveVersion](*this);
	}
	return bRan;
}

void UDISaveGame::MarkNew()
{
	SaveVersion = CurrentVersion;
	EffectsVolume = UDIGameSettings::Get().DefaultEffectsVolume;
	AmbienceVolume = UDIGameSettings::Get().DefaultAmbienceVolume;
	Owned = UDIGameSettings::Get().StartingPieces;
}
