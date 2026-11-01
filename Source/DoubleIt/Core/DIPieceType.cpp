#include "DIPieceType.h"
#include "DIMode.h"

namespace
{
	class FDINumberType final : public IDIPieceType
	{
	public:
		virtual FName Name() const override { return TEXT("Number"); }
	};

	class FDIPlusType final : public IDIPieceType
	{
	public:
		virtual FName Name() const override { return TEXT("Plus"); }
		virtual bool IsCatalyst() const override { return true; }
	};

	class FDIMinusType final : public IDIPieceType
	{
	public:
		virtual FName Name() const override { return TEXT("Minus"); }
		virtual bool LiftsFromRing() const override { return true; }
		virtual TOptional<EDIPieceKind> LiftedConvertsTo() const override { return EDIPieceKind::Plus; }
	};

	/**
	 * Owned, never dealt. Clears the numbers in the half of the ring nearest to where it is placed (a share of the ring
	 * from the mode config, taken outward from the tap on both sides); pluses and minuses there stay. Not placed itself.
	 */
	class FDIBreezeType final : public IDIPieceType
	{
	public:
		virtual FName Name() const override { return TEXT("Breeze"); }
		virtual bool IsOwned() const override { return true; }
		virtual bool InsertsOnPlace() const override { return false; }
		virtual FName LandCue() const override { return TEXT("Breeze"); }

		virtual void OnPlaced(TArray<FDIPiece>& Ring, int32 Slot, const FDIModeConfig& Config) const override
		{
			const int32 Num = Ring.Num();
			const int32 Reach = FMath::FloorToInt32(Num * Config.BreezeClearFraction);
			TArray<int32> Cleared;
			for (int32 k = 0; Cleared.Num() < Reach; ++k)
			{
				// Nearest first, alternating sides: the stone after the gap, the one before it, then one further out each.
				const int32 Index = k % 2 == 0 ? Slot + k / 2 : Slot - 1 - k / 2;
				Cleared.Add(((Index % Num) + Num) % Num);
			}
			Cleared.Sort([](int32 A, int32 B) { return A > B; });
			for (const int32 Index : Cleared)
			{
				if (Ring[Index].IsNumber())
				{
					Ring.RemoveAt(Index);
				}
			}
		}
	};

	TMap<EDIPieceKind, TSharedRef<const IDIPieceType>>& PieceRegistry()
	{
		static TMap<EDIPieceKind, TSharedRef<const IDIPieceType>> Types = []
		{
			TMap<EDIPieceKind, TSharedRef<const IDIPieceType>> Built;
			Built.Add(EDIPieceKind::Number, MakeShared<FDINumberType>());
			Built.Add(EDIPieceKind::Plus, MakeShared<FDIPlusType>());
			Built.Add(EDIPieceKind::Minus, MakeShared<FDIMinusType>());
			Built.Add(EDIPieceKind::Breeze, MakeShared<FDIBreezeType>());
			return Built;
		}();
		return Types;
	}
}

const IDIPieceType& DIPieces::Get(EDIPieceKind Kind)
{
	const TSharedRef<const IDIPieceType>* Found = PieceRegistry().Find(Kind);
	return Found ? Found->Get() : PieceRegistry().FindChecked(EDIPieceKind::Number).Get();
}

void DIPieces::Register(EDIPieceKind Kind, TSharedRef<const IDIPieceType> Type)
{
	PieceRegistry().Add(Kind, Type);
}

TOptional<EDIPieceKind> DIPieces::Find(FName Name)
{
	for (const TPair<EDIPieceKind, TSharedRef<const IDIPieceType>>& Type : PieceRegistry())
	{
		if (Type.Value->Name() == Name)
		{
			return Type.Key;
		}
	}
	return {};
}
