#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DITypes.h"
#include "DIRingActor.generated.h"

class FDIGame;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Draws the board: one instanced quad per visible piece and a full-screen background quad.
 * Visuals are keyed by piece id, so pieces glide when the ring reflows, fused pieces glide into
 * the merge point and fade, and fusion results pulse in.
 * Per-instance custom data feeds M_Tile: [0] value, [1] kind, [2] colour rank, [3] alpha.
 */
UCLASS()
class DOUBLEIT_API ADIRingActor : public AActor
{
	GENERATED_BODY()

public:
	ADIRingActor();

	void SyncFromGame(const FDIGame& Game);
	void SetDark(bool bDark);

	/** Screen-aligned 2D board space <-> world. The camera looks down -Z with +X as screen-up. */
	static FVector ToWorld(const FVector2D& P, float Z) { return FVector(P.Y, P.X, Z); }
	static FVector2D FromWorld(const FVector& W) { return FVector2D(W.Y, W.X); }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FTileVisual
	{
		FDIPiece Piece;
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D TargetPos = FVector2D::ZeroVector;
		float Size = 0.f;
		float TargetSize = 0.f;
		float Alpha = 1.f;
		float TargetAlpha = 1.f;
		bool bCenter = false;
		bool bDying = false;
	};

	void PushInstances();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Background;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Tiles;

	TArray<FTileVisual> Visuals;
	EDIRuleSet Rules = EDIRuleSet::Doubling;
	bool bAnimating = false;
};
