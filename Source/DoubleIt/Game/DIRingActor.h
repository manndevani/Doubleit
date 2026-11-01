#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DITypes.h"
#include "DIRingActor.generated.h"

class FDIGame;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Draws the board: one instanced quad per piece (index 0 = center) and a full-screen background quad.
 * Per-instance custom data feeds M_Tile: [0] value, [1] kind, [2] colour rank, [3] alpha.
 */
UCLASS()
class DOUBLEIT_API ADIRingActor : public AActor
{
	GENERATED_BODY()

public:
	ADIRingActor();

	void SyncFromGame(const FDIGame& Game);

	/** Screen-aligned 2D board space <-> world. The camera looks down -Z with +X as screen-up. */
	static FVector ToWorld(const FVector2D& P, float Z) { return FVector(P.Y, P.X, Z); }
	static FVector2D FromWorld(const FVector& W) { return FVector2D(W.Y, W.X); }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FTileVisual
	{
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D TargetPos = FVector2D::ZeroVector;
		float Size = 0.f;
		float TargetSize = 0.f;
	};

	void WriteCustomData(int32 Instance, const FDIPiece& Piece, EDIRuleSet Rules);
	void PushTransforms();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Background;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Tiles;

	TArray<FTileVisual> Visuals;
};
