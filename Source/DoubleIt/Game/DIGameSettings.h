#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Core/DIMode.h"
#include "DIGameSettings.generated.h"

/**
 * Feel and layout numbers, editable in Project Settings > Game > Double It and saved to Config/DefaultGame.ini.
 * Rule numbers live in each mode's FDIModeConfig (Core/DIMode.h); MakeMode applies per-mode overrides from the
 * [DoubleIt.Mode.<Key>] sections of DefaultGame.ini. Read with UDIGameSettings::Get().
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Double It"))
class DOUBLEIT_API UDIGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UDIGameSettings& Get() { return *GetDefault<UDIGameSettings>(); }

	/** The registered mode for Key with its DefaultGame.ini overrides applied. */
	static FDIMode MakeMode(FName Key);

	UPROPERTY(Config, EditAnywhere, Category = "Run")
	FName DefaultMode = DIModes::Classic;

	/** Move playback: the piece lands, then one collision per step, then the next centre appears. */
	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float FirstCollisionDelay = 0.16f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float ChainStepDelay = 0.22f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float SettleDelay = 0.14f;

	/** Springs: stiffness 230 settles in ~0.45 s; damping 23 is a ratio of ~0.75 (~3% overshoot). */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float SpringStiffness = 230.f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float SpringDamping = 23.f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion", meta = (ClampMin = 1))
	int32 SpringSubsteps = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float FadeRate = 7.f;

	/** A fusion result starts at this size and springs back to 1. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float PulseScale = 1.22f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion", meta = (Units = "s"))
	float RippleSeconds = 0.55f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float AimDotSize = 9.f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float AimDotSpacing = 26.f;

	/** Camera fit: world units across on a phone; we only ever zoom out from it. */
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	float PhoneWidthUnits = 820.f;

	/** Ring radius + half a tile + a little air, world units. */
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	float RingExtentUnits = 345.f;

	/** Score, best and two tutorial lines (+ air), UMG units from the top. */
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	float TopBandUnits = 500.f;

	/** The game-over lines start at this share of the screen height. */
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	float BottomBandFraction = 0.8f;

	/** Caps zoom on big tablets so stones don't get huge. */
	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	float MaxPixelsPerUnit = 1.6f;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
