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
 * Visuals are keyed by piece id and animated in polar space: reflows and merges slide along the
 * ring arc, placements and lifts travel radially, fused pieces fade, and fusion results pulse in.
 * Per-instance custom data feeds M_Tile: [0] value, [1] kind, [2] colour rank, [3] alpha.
 */
UCLASS()
class DOUBLEIT_API ADIRingActor : public AActor
{
	GENERATED_BODY()

public:
	ADIRingActor();

	/** Shows the current game state at ring offset Offset (see DIRingLayout::Reanchor). */
	void SyncFromGame(const FDIGame& Game, float Offset);

	/**
	 * Shows one step of a move: Ring at Offset, an optional centre piece, and optionally the fusion event that produced
	 * this ring (its consumed pieces collapse into the result from both sides, the result pulses in).
	 */
	void ShowBoard(const TArray<FDIPiece>& Ring, const FDIPiece* Center, float Offset, const FDIFusionEvent* Event, EDIRuleSet InRules);

	/** Expanding ring at a board angle; Strength 0..1 scales size and shake (shake only above 0.6). */
	void Impact(float Angle, float Strength, bool bReveal);
	void SetDark(bool bDark);
	bool IsAnimating() const { return bAnimating; }

	/** Radial dotted guide from the centre to Angle: a gap between two tiles, or the tile a minus would lift. */
	void SetAim(float Angle, bool bAtTile);
	void ClearAim();

	/** Screen-aligned 2D board space <-> world. The camera looks down -Z with +X as screen-up. */
	static FVector ToWorld(const FVector2D& P, float Z) { return FVector(P.Y, P.X, Z); }
	static FVector2D FromWorld(const FVector& W) { return FVector2D(W.Y, W.X); }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Polar state (clockwise angle from the top, radius from the centre) so motion follows the ring, never cuts across it. */
	struct FTileVisual
	{
		FDIPiece Piece;
		float Angle = 0.f;
		float TargetAngle = 0.f;
		float Radius = 0.f;
		float TargetRadius = 0.f;
		float Size = 0.f;
		float TargetSize = 0.f;
		float AngleVel = 0.f;  // springs: pieces overshoot a touch and settle, like stones dropped in water
		float RadiusVel = 0.f;
		float SizeVel = 0.f;
		float Seed = 0.f;      // 0..1 from the piece id: pebble outline and breathing phase
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

	struct FRipple
	{
		FVector2D Pos;
		float Age = 0.f;
		float MaxSize = 0.f;
		bool bReveal = false;
	};

	TArray<FTileVisual> Visuals;
	TArray<FRipple> Ripples;
	float ShakeAmp = 0.f;
	FVector2D ShakeOffset = FVector2D::ZeroVector;
	float AimAngle = 0.f;
	float AimAlpha = 0.f;
	float AimTargetAlpha = 0.f;
	bool bAimAtTile = false;
	EDIRuleSet Rules = EDIRuleSet::Doubling;
	bool bAnimating = false;
};
