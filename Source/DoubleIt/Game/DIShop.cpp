#include "DIShop.h"
#include "Core/DIGame.h"
#include "Core/DIPieceType.h"
#include "Game/DIGameEvents.h"
#include "Game/DIGameSettings.h"
#include "Game/DISaveGame.h"
#include "Game/DoubleItGameMode.h"
#include "Kismet/GameplayStatics.h"

void UDIShop::Init(ADoubleItGameMode& InGameMode, UDISaveGame& InSave, UDIGameEvents& InEvents)
{
	GameMode = &InGameMode;
	Save = &InSave;
	Events = &InEvents;
	Wallet.Owned = Save->Owned;
	Wallet.RewardedUpTo = Save->RewardedUpTo;
	Events->StateChanged.AddUObject(this, &UDIShop::OnStateChanged);
	Events->PiecePlaced.AddUObject(this, &UDIShop::OnPiecePlaced);

	Store = DIStores::Make();
	TArray<FName> Ids;
	for (const FDIPiecePack& Pack : UDIGameSettings::Get().Packs)
	{
		Ids.Add(Pack.ProductId);
	}
	Store->Query(Ids, [WeakThis = TWeakObjectPtr<UDIShop>(this)](TArray<FDIStoreProduct> Found)
	{
		if (UDIShop* Self = WeakThis.Get())
		{
			Self->Products = MoveTemp(Found);
			Self->Events->StateChanged.Broadcast();
		}
	});
}

void UDIShop::Buy(FName ProductId)
{
	const FDIPiecePack* Pack = UDIGameSettings::Get().Packs.FindByPredicate([ProductId](const FDIPiecePack& P) { return P.ProductId == ProductId; });
	if (!Pack || !Store)
	{
		return;
	}
	const FDIPiecePack Bought = *Pack;
	Store->Buy(ProductId, [WeakThis = TWeakObjectPtr<UDIShop>(this), Bought](bool bPaid)
	{
		UDIShop* Self = WeakThis.Get();
		if (!Self || !bPaid)
		{
			return;
		}
		Self->Wallet.Add(Bought.Piece, Bought.Count);
		Self->Persist();
		Self->Events->PiecesBought.Broadcast(Bought.Count);
	});
}

bool UDIShop::Counts() const
{
	const ADoubleItGameMode* GM = GameMode.Get();
	return GM && GM->GetGame() && !GM->IsInTutorial() && !GM->IsAutomationRun();
}

void UDIShop::OnStateChanged()
{
	// The settled board: a first-ever number pays out even if the chain that made it was fast-forwarded.
	if (!Counts())
	{
		return;
	}
	const FDIGame& Game = *GameMode->GetGame();
	if (Wallet.Reached(Game.GetMode().Key, Game.GetHighestValue(), Game.GetMode().Config))
	{
		Persist();
		Events->PieceEarned.Broadcast(Game.GetHighestValue());
	}
}

void UDIShop::OnPiecePlaced(const FDIPiece& Piece, float Angle)
{
	if (Counts() && DIPieces::Of(Piece).IsOwned() && Wallet.Spend(DIPieces::Of(Piece).Name()))
	{
		Persist();
	}
}

void UDIShop::Persist()
{
	Save->Owned = Wallet.Owned;
	Save->RewardedUpTo = Wallet.RewardedUpTo;
	UGameplayStatics::SaveGameToSlot(Save, UDISaveGame::SlotName, 0);
	Events->StateChanged.Broadcast();
}
