#include "DIGame.h"
#include "DIPieceType.h"

FDIGame::FDIGame(const FDIMode& InMode, int32 Seed)
	: Mode(InMode)
	, Spawner(InMode.Rules.ToSharedRef(), Seed, InMode.Config.Spawn)
{
	for (int32 i = 0; i < Mode.Config.StartRingPieces; ++i)
	{
		FDIPiece Piece = Spawner.Next(0, Ring.Num());
		while (!Piece.IsNumber())
		{
			Piece = Spawner.Next(0, Ring.Num());
		}
		Ring.Add(Tag(Piece));
	}
	Center = Tag(Spawner.Next(0, Ring.Num()));
}

void FDIGame::SetBoard(const TArray<FDIPiece>& InRing, const FDIPiece& InCenter)
{
	SetRingForTest(InRing);
	SetCenterForTest(InCenter);
	LastEvents.Reset();
	bGameOver = false;
	Waiting.Reset();
	LastPlaced = EDIPieceKind::Number;
}

void FDIGame::SetRingForTest(const TArray<FDIPiece>& InRing)
{
	Ring.Reset();
	for (const FDIPiece& Piece : InRing)
	{
		Ring.Add(Tag(Piece));
	}
}

bool FDIGame::PlaceAt(int32 Slot)
{
	const IDIPieceType& Type = DIPieces::Of(Center);
	if (bGameOver || Type.LiftsFromRing() || Slot < 0 || Slot > Ring.Num())
	{
		return false;
	}

	LastEvents.Reset();
	if (Type.InsertsOnPlace())
	{
		Ring.Insert(Center, Slot);
		if (Center.IsNumber())
		{
			HighestValue = FMath::Max(HighestValue, Center.Value);
		}
	}
	Type.OnPlaced(Ring, Slot, Mode.Config);
	LastPlaced = Center.Kind;

	PlacedRing = Ring;
	ResolveAll();
	FinishMove();
	return true;
}

bool FDIGame::AbsorbAt(int32 Index)
{
	if (bGameOver || !DIPieces::Of(Center).LiftsFromRing() || !Ring.IsValidIndex(Index))
	{
		return false;
	}
	LiftConvertsTo = DIPieces::Of(Center).LiftedConvertsTo();
	Center = Ring[Index];
	Ring.RemoveAt(Index);
	PlacedRing = Ring;
	LastEvents.Reset();
	bCenterAbsorbed = LiftConvertsTo.IsSet();
	return true;
}

bool FDIGame::ConvertCenterToPlus()
{
	if (bGameOver || !bCenterAbsorbed)
	{
		return false;
	}
	Center = Tag(FDIPiece{ LiftConvertsTo.GetValue(), 0, 0 });
	bCenterAbsorbed = false;
	return true;
}

bool FDIGame::BringIn(EDIPieceKind Kind)
{
	if (bGameOver || Waiting.IsSet() || !DIPieces::Get(Kind).IsOwned())
	{
		return false;
	}
	Waiting = FWaiting{ Center, bCenterAbsorbed, LiftConvertsTo };
	Center = Tag(FDIPiece{ Kind, 0, 0 });
	bCenterAbsorbed = false;
	return true;
}

bool FDIGame::ReturnHeld()
{
	if (!Waiting.IsSet())
	{
		return false;
	}
	Center = Waiting->Center;
	bCenterAbsorbed = Waiting->bAbsorbed;
	LiftConvertsTo = Waiting->ConvertsTo;
	Waiting.Reset();
	return true;
}

void FDIGame::ResolveAll()
{
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (int32 i = 0; i < Ring.Num(); ++i)
		{
			if (DIPieces::Of(Ring[i]).IsCatalyst() && TryFuseAt(i))
			{
				bChanged = true;
				break;
			}
		}
	}
}

bool FDIGame::TryFuseAt(int32 PlusIndex)
{
	int32 CenterIndex = PlusIndex;
	int32 Depth = 0;

	while (Ring.Num() >= 3)
	{
		const int32 Num = Ring.Num();
		const FDIPiece& L = Ring[(CenterIndex - 1 + Num) % Num];
		const FDIPiece& R = Ring[(CenterIndex + 1) % Num];
		if (!L.IsNumber() || !R.IsNumber() || L.Value != R.Value)
		{
			break;
		}

		const int64 Outer = L.Value;
		const int64 NewValue = Depth == 0
			? Mode.Rules->FirstFusion(Outer)
			: Mode.Rules->ChainFusion(Ring[CenterIndex].Value, Outer);

		FDIFusionEvent Event;
		Event.ConsumedIds[0] = L.Id;
		Event.ConsumedIds[1] = R.Id;
		Event.ConsumedIds[2] = Ring[CenterIndex].Id;

		RemoveNeighbours(CenterIndex);
		Ring[CenterIndex] = Tag(FDIPiece::Number(NewValue));
		++Depth;

		Score += Mode.Rules->FusionScore(NewValue, Depth);
		HighestValue = FMath::Max(HighestValue, NewValue);
		Event.ChainDepth = Depth;
		Event.ResultValue = NewValue;
		Event.ResultId = Ring[CenterIndex].Id;
		Event.RingAfter = Ring;
		LastEvents.Add(Event);
	}
	return Depth > 0;
}

void FDIGame::RemoveNeighbours(int32& CenterIndex)
{
	const int32 Num = Ring.Num();
	const int32 L = (CenterIndex - 1 + Num) % Num;
	const int32 R = (CenterIndex + 1) % Num;
	const int32 Hi = FMath::Max(L, R);
	const int32 Lo = FMath::Min(L, R);

	Ring.RemoveAt(Hi);
	if (Hi < CenterIndex)
	{
		--CenterIndex;
	}
	Ring.RemoveAt(Lo);
	if (Lo < CenterIndex)
	{
		--CenterIndex;
	}
}

void FDIGame::FinishMove()
{
	++Moves;
	bCenterAbsorbed = false;
	if (Ring.Num() > Mode.Config.MaxRingPieces)
	{
		bGameOver = true;
		return;
	}
	if (Waiting.IsSet())
	{
		ReturnHeld(); // the piece that waited behind the brought-in one is next
		return;
	}
	Center = Tag(Spawner.Next(Moves, Ring.Num()));
}

bool FDIGame::SerializeRun(FArchive& Ar)
{
	int32 Format = RunFormat;
	Ar << Format;
	if (Ar.IsError() || Format < 1 || Format > RunFormat)
	{
		return false;
	}
	Ar << Ring << Center << Score << HighestValue << Moves << NextId << bGameOver << bCenterAbsorbed;
	bool bConverts = LiftConvertsTo.IsSet();
	uint8 ConvertsTo = bConverts ? uint8(LiftConvertsTo.GetValue()) : 0;
	Ar << bConverts << ConvertsTo;
	Spawner.Serialize(Ar);
	if (Format >= 2)
	{
		bool bWaiting = Waiting.IsSet();
		FWaiting W = bWaiting ? Waiting.GetValue() : FWaiting();
		bool bWaitConverts = W.ConvertsTo.IsSet();
		uint8 WaitConvertsTo = bWaitConverts ? uint8(W.ConvertsTo.GetValue()) : 0;
		Ar << bWaiting << W.Center << W.bAbsorbed << bWaitConverts << WaitConvertsTo;
		if (Ar.IsLoading())
		{
			W.ConvertsTo = bWaitConverts ? TOptional<EDIPieceKind>(EDIPieceKind(WaitConvertsTo)) : TOptional<EDIPieceKind>();
			Waiting = bWaiting ? TOptional<FWaiting>(W) : TOptional<FWaiting>();
		}
	}
	if (Ar.IsLoading())
	{
		LiftConvertsTo = bConverts ? TOptional<EDIPieceKind>(EDIPieceKind(ConvertsTo)) : TOptional<EDIPieceKind>();
		Mode.Config.Spawn = Spawner.GetTuning();
		LastEvents.Reset();
		PlacedRing = Ring;
	}
	return !Ar.IsError() && (!Ar.IsLoading() || Ring.Num() <= Mode.Config.MaxRingPieces + 1);
}
