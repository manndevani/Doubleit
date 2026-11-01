#include "DIStore.h"

namespace
{
	/** Nothing to sell yet: the shop says it opens soon. */
	class FDIClosedStore final : public IDIStore
	{
	public:
		virtual void Query(const TArray<FName>& Ids, TFunction<void(TArray<FDIStoreProduct>)> Done) override { Done({}); }
		virtual void Buy(FName Id, TFunction<void(bool)> Done) override { Done(false); }
	};

#if !UE_BUILD_SHIPPING
	/** Development only: sells everything at a test price, instantly, so the shop can be tried without Play. */
	class FDIFakeStore final : public IDIStore
	{
	public:
		virtual void Query(const TArray<FName>& Ids, TFunction<void(TArray<FDIStoreProduct>)> Done) override
		{
			TArray<FDIStoreProduct> Products;
			for (const FName& Id : Ids)
			{
				Products.Add({ Id, FText::AsCultureInvariant(TEXT("test")) });
			}
			Done(MoveTemp(Products));
		}
		virtual void Buy(FName Id, TFunction<void(bool)> Done) override { Done(true); }
	};
#endif
}

TSharedRef<IDIStore> DIStores::Make()
{
#if UE_BUILD_SHIPPING
	return MakeShared<FDIClosedStore>();
#else
	return MakeShared<FDIFakeStore>();
#endif
}
