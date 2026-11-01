#include "DIGameSettings.h"
#include "Misc/ConfigCacheIni.h"

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
	Int(TEXT("PlusGuaranteeWindow"), S.PlusGuaranteeWindow);
	Float(TEXT("PlusChance"), S.PlusChance);
	Float(TEXT("MinusChance"), S.MinusChance);
	Int(TEXT("MinusFromMove"), S.MinusFromMove);
	Float(TEXT("BreezeChance"), S.BreezeChance);
	Int(TEXT("BreezeFromMove"), S.BreezeFromMove);
	Int(TEXT("BreezeMinRing"), S.BreezeMinRing);

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
