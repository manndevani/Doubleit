#pragma once

#include "CoreMinimal.h"

/** One product the store can sell now, with its price in the player's currency. */
struct FDIStoreProduct
{
	FName Id;
	FText Price;
};

/**
 * Where purchases go (Strategy). UDIShop only talks to this. DIStores::Make picks the implementation: a fake store in
 * development builds (every product, instant success) and a closed one in shipping builds until Google Play Billing
 * is wired up behind this same interface. See docs/MONETIZATION.md.
 */
class IDIStore
{
public:
	virtual ~IDIStore() = default;

	/** The products of Ids this store can sell now. Done gets an empty list if the store is closed or offline. */
	virtual void Query(const TArray<FName>& Ids, TFunction<void(TArray<FDIStoreProduct>)> Done) = 0;

	/** Buys one of Id. Done(true) only once it is paid for. */
	virtual void Buy(FName Id, TFunction<void(bool)> Done) = 0;
};

namespace DIStores
{
	TSharedRef<IDIStore> Make();
}
