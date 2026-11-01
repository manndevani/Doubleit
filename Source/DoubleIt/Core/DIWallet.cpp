#include "DIWallet.h"
#include "DIMode.h"

void FDIWallet::Add(FName Piece, int32 Amount)
{
	Owned.FindOrAdd(Piece) = FMath::Max(Count(Piece) + Amount, 0);
}

bool FDIWallet::Spend(FName Piece)
{
	if (Count(Piece) <= 0)
	{
		return false;
	}
	Add(Piece, -1);
	return true;
}

bool FDIWallet::Reached(FName Mode, int64 Value, const FDIModeConfig& Config)
{
	int64& Paid = RewardedUpTo.FindOrAdd(Mode);
	if (Value < Config.RewardFrom || Value <= Paid)
	{
		return false;
	}
	Paid = Value;
	Add(Config.RewardPiece, 1);
	return true;
}
