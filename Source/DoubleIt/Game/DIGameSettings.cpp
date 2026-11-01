#include "DIGameSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Core/DIPieceType.h"

FDIMode UDIGameSettings::MakeMode(FName Key)
{
	FDIMode Mode = DIModes::Get(Key);
	if (!GConfig)
	{
		return Mode;
	}

	// Only keys present in the section override the code defaults; names match the FDIModeConfig fields.
	const FString Section = FString::Printf(TEXT("DoubleIt.Mode.%s"), *Mode.Key.ToString());
	auto Int = [&](const TCHAR* Name, int32& Value) { GConfig->GetInt(*Section, Name, Value, GGameIni); };
	auto Float = [&](const TCHAR* Name, float& Value) { GConfig->GetFloat(*Section, Name, Value, GGameIni); };

	FDIModeConfig& C = Mode.Config;
	Int(TEXT("StartRingPieces"), C.StartRingPieces);
	Int(TEXT("MaxRingPieces"), C.MaxRingPieces);
	Float(TEXT("BreezeClearFraction"), C.BreezeClearFraction);
	GConfig->GetInt64(*Section, TEXT("RevealFrom"), C.RevealFrom, GGameIni);

	FDISpawnTuning& S = C.Spawn;
	Int(TEXT("MovesPerRangeStep"), S.MovesPerRangeStep);
	for (FDISpawnRow& Row : S.Specials)
	{
		// Rows are keyed by piece type name: PlusChance, MinusFromMove, BreezeMinRing, PlusGuaranteeWindow, MinusNoRepeat...
		const FString Name = DIPieces::Get(Row.Kind).Name().ToString();
		Float(*(Name + TEXT("Chance")), Row.Chance);
		Int(*(Name + TEXT("FromMove")), Row.FromMove);
		Int(*(Name + TEXT("MinRing")), Row.MinRing);
		Int(*(Name + TEXT("GuaranteeWindow")), Row.GuaranteeWindow);
		GConfig->GetBool(*Section, *(Name + TEXT("NoRepeat")), Row.bNoRepeat, GGameIni);
	}

	FDIAdaptiveTuning& A = C.Adaptive;
	Float(TEXT("LearnRate"), A.LearnRate);
	Float(TEXT("MinPlusChance"), A.MinPlusChance);
	Float(TEXT("MaxPlusChance"), A.MaxPlusChance);
	Int(TEXT("MinGrowthMoves"), A.MinGrowthMoves);
	Int(TEXT("MaxGrowthMoves"), A.MaxGrowthMoves);
	Float(TEXT("HelpPlusChance"), A.HelpPlusChance);
	Float(TEXT("PushPlusChance"), A.PushPlusChance);
	return Mode;
}

UDIGameSettings::UDIGameSettings()
{
	// Default cues (steps on the current chord of Progression, see Core/DISynth). DefaultGame.ini may override any of them.
	auto Cue = [this](const TCHAR* Name, TArray<FDICueNote> Notes, int32 HapticMs = 0) -> FDISoundCue&
	{
		FDISoundCue& C = Cues.Add(Name);
		C.Notes = MoveTemp(Notes);
		C.HapticMs = HapticMs;
		return C;
	};
	Cue(TEXT("Place"), { { -6, 0.2f, 0.f } }, 8).bLevelFromAngle = true;
	FDISoundCue& Chain = Cue(TEXT("ChainStep"), { { 1, 0.34f, 0.f } }, 14); // each collision a step higher
	Chain.HapticPerLevel = 6;
	Chain.HapticLevelCap = 5;
	Cue(TEXT("ChainSettle"), { { -3, 0.14f, 0.05f } });
	Cue(TEXT("Breeze"), { { 7, 0.16f, 0.f }, { 5, 0.16f, 0.06f }, { 3, 0.16f, 0.12f }, { 1, 0.16f, 0.18f }, { -1, 0.16f, 0.24f } }, 16);
	Cue(TEXT("Reveal"), { { 5, 0.26f, 0.05f }, { 7, 0.26f, 0.14f }, { 9, 0.3f, 0.23f } });
	Cue(TEXT("NewBest"), { { 2, 0.18f, 0.f }, { 4, 0.18f, 0.03f }, { 7, 0.2f, 0.06f } }, 24);
	Cue(TEXT("Lift"), { { 3, 0.18f, 0.f }, { 1, 0.16f, 0.07f } }, 8);
	Cue(TEXT("GameOver"), { { 4, 0.25f, 0.f }, { 2, 0.25f, 0.3f }, { 0, 0.25f, 0.6f } }, 40);
	Cue(TEXT("TutorialStep"), { { 2, 0.3f, 0.f }, { 4, 0.3f, 0.1f } });
	Cue(TEXT("HapticsOn"), {}, 20);

	// Five-tone chords of C major (C D E G B, A C E G B...), so the steps keep the old pentatonic spacing and any
	// order of moves lands in harmony. I - vi - IV - V, all with soft added notes, nothing to resolve urgently.
	Progression = {
		{ TEXT("Cmaj9"), { 0, 2, 4, 7, 11 } },
		{ TEXT("Am9"), { 0, 4, 7, 9, 11 } },
		{ TEXT("Fmaj9"), { 0, 4, 5, 7, 9 } },
		{ TEXT("G6add9"), { 2, 4, 7, 9, 11 } },
	};
}
