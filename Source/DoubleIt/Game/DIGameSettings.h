#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Core/DIMode.h"
#include "DIGameSettings.generated.h"

/** One note of a sound cue: pentatonic step (DISynth), volume 0..1, delay from the cue start. */
USTRUCT()
struct FDICueNote
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Cue") int32 Step = 0;
	UPROPERTY(Config, EditAnywhere, Category = "Cue") float Volume = 0.2f;
	UPROPERTY(Config, EditAnywhere, Category = "Cue", meta = (Units = "s")) float Delay = 0.f;
};

/**
 * A sound + haptic cue. Played with a level (chain depth, ring zone): every note is raised by the level, and the
 * haptic lasts HapticMs + HapticPerLevel * min(level, HapticLevelCap) ms.
 */
USTRUCT()
struct FDISoundCue
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Cue") TArray<FDICueNote> Notes;
	UPROPERTY(Config, EditAnywhere, Category = "Cue") int32 HapticMs = 0;
	UPROPERTY(Config, EditAnywhere, Category = "Cue") int32 HapticPerLevel = 0;
	UPROPERTY(Config, EditAnywhere, Category = "Cue") int32 HapticLevelCap = 0;
	/** Level comes from where the piece landed on the ring (one of PlaceZones), so moves sound musical. */
	UPROPERTY(Config, EditAnywhere, Category = "Cue") bool bLevelFromAngle = false;
};

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

	UDIGameSettings();

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

	/** Impact ripples grow to this many tile sizes: Min..Max by chain strength, Reveal for a first-ever number. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float RippleSizeMin = 1.8f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float RippleSizeMax = 2.6f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float RevealRippleSize = 3.4f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float RippleOpacity = 0.7f;

	/** Impacts stronger than this (0..1; chains of 3+) shake the board by ShakePixelsMin..Max. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float ShakeFrom = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float ShakePixelsMin = 2.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float ShakePixelsMax = 3.5f;

	/** Consumed pieces shrink to this share of their size as they slide in and fade. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float ConsumedShrink = 0.6f;

	/** The aim guide's last dot (the landing gap) is this much larger; dots fade from Near to Far opacity. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float AimTipScale = 1.8f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float AimOpacityNear = 0.3f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float AimOpacityFar = 0.85f;

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

	/** Sound and haptic cues by name. Pieces name their landing cue (IDIPieceType::LandCue); events name the rest. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TMap<FName, FDISoundCue> Cues;

	/** Ring zones for landing notes: clockwise from the top climbs one step per zone. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = 1))
	int32 PlaceZones = 8;

	/** Chains this long or longer end with the ChainSettle cue. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	int32 ChainSettleFrom = 2;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
