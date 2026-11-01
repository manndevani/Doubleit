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

	/** Clears the lowest numbers (a share of the ring from the mode config) and is not placed itself. */
	class FDIBreezeType final : public IDIPieceType
	{
	public:
		virtual FName Name() const override { return TEXT("Breeze"); }
		virtual bool InsertsOnPlace() const override { return false; }
		virtual FName LandCue() const override { return TEXT("Breeze"); }

		virtual void OnPlaced(TArray<FDIPiece>& Ring, const FDIModeConfig& Config) const override
		{
			int32 ToRemove = FMath::FloorToInt32(Ring.Num() * Config.BreezeClearFraction);
			while (ToRemove-- > 0)
			{
				int32 Lowest = INDEX_NONE;
				for (int32 i = 0; i < Ring.Num(); ++i)
				{
					if (Ring[i].IsNumber() && (Lowest == INDEX_NONE || Ring[i].Value < Ring[Lowest].Value))
					{
						Lowest = i;
					}
				}
				if (Lowest == INDEX_NONE)
				{
					break;
				}
				Ring.RemoveAt(Lowest);
			}
		}
	};

	TMap<EDIPieceKind, TSharedRef<const IDIPieceType>>& Registry()
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
	const TSharedRef<const IDIPieceType>* Found = Registry().Find(Kind);
	return Found ? Found->Get() : Registry().FindChecked(EDIPieceKind::Number).Get();
}

void DIPieces::Register(EDIPieceKind Kind, TSharedRef<const IDIPieceType> Type)
{
	Registry().Add(Kind, Type);
}
