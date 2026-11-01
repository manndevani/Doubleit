#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/DIWallet.h"
#include "Game/DIStore.h"
#include "DIShop.generated.h"

class ADoubleItGameMode;
class UDIGameEvents;
class UDISaveGame;
struct FDIPiece;

/**
 * Owned pieces (the breeze): keeps the FDIWallet in the save, pays rewards for first-ever numbers, spends a piece
 * when one is used, and sells packs through an IDIStore. Reacts to UDIGameEvents only; tutorial and automation runs
 * neither earn nor spend. Spec: docs/GAME_DESIGN.md#breeze
 */
UCLASS()
class DOUBLEIT_API UDIShop : public UObject
{
	GENERATED_BODY()

public:
	void Init(ADoubleItGameMode& InGameMode, UDISaveGame& InSave, UDIGameEvents& InEvents);

	int32 Count(FName Piece) const { return Wallet.Count(Piece); }

	/** Packs the store can sell now (in UDIGameSettings::Packs order), with prices. Empty: the shop is not open. */
	const TArray<FDIStoreProduct>& GetProducts() const { return Products; }

	/** Buys the pack with this product id; the pieces arrive once the store confirms payment. */
	void Buy(FName ProductId);

private:
	void OnStateChanged();
	void OnPiecePlaced(const FDIPiece& Piece, float Angle);
	bool Counts() const;
	void Persist();

	TWeakObjectPtr<ADoubleItGameMode> GameMode;
	UPROPERTY() TObjectPtr<UDISaveGame> Save;
	UPROPERTY() TObjectPtr<UDIGameEvents> Events;
	FDIWallet Wallet;
	TSharedPtr<IDIStore> Store;
	TArray<FDIStoreProduct> Products;
};
