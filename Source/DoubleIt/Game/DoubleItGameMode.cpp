#include "DoubleItGameMode.h"
#include "DoubleIt.h"

void ADoubleItGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartNewGame(EDIGameModeRules::Doubling, FMath::Rand());
	UE_LOG(LogDoubleIt, Log, TEXT("Ready"));
}

void ADoubleItGameMode::StartNewGame(EDIGameModeRules Rules, int32 Seed)
{
	Game = MakeUnique<FDIGame>(Rules == EDIGameModeRules::Addition ? EDIRuleSet::Addition : EDIRuleSet::Doubling, Seed);
	OnBoardChanged.Broadcast();
}

bool ADoubleItGameMode::PlaceAt(int32 Slot)
{
	return Broadcast(Game && Game->PlaceAt(Slot));
}

bool ADoubleItGameMode::AbsorbAt(int32 Index)
{
	return Broadcast(Game && Game->AbsorbAt(Index));
}

bool ADoubleItGameMode::ConvertCenterToPlus()
{
	return Broadcast(Game && Game->ConvertCenterToPlus());
}

bool ADoubleItGameMode::Broadcast(bool bChanged)
{
	if (bChanged)
	{
		OnBoardChanged.Broadcast();
	}
	return bChanged;
}
