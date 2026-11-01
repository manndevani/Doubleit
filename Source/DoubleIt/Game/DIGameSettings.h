#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Core/DIMode.h"
#include "DIGameSettings.generated.h"

/** A chord the melody follows: its tones as semitones above the harmony root, within one octave, sorted. */
USTRUCT()
struct FDIChord
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Harmony") FName Name;
	UPROPERTY(Config, EditAnywhere, Category = "Harmony") TArray<int32> Tones;
};

/** One note of a sound cue: a step on the current chord (step 0 = its first tone), volume 0..1, delay from the cue start. */
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

	/**
	 * Move playback: the piece travels out and lands, then one collision per step, then the next centre appears.
	 * The first collision waits until the placed pebble has reached the ring (within LandTolerance, checked every
	 * LandCheckSeconds, at most LandWaitMax), then a beat of FirstCollisionDelay.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float FirstCollisionDelay = 0.14f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback")
	float LandTolerance = 4.f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float LandCheckSeconds = 0.02f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float LandWaitMax = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float ChainStepDelay = 0.34f;

	UPROPERTY(Config, EditAnywhere, Category = "Playback", meta = (Units = "s"))
	float SettleDelay = 0.2f;

	/** Springs: stiffness 160 settles in ~0.55 s; damping 19 is a ratio of ~0.75 (~3% overshoot). */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float SpringStiffness = 160.f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float SpringDamping = 19.f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion", meta = (ClampMin = 1))
	int32 SpringSubsteps = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float FadeRate = 5.f;

	/** A fusion result starts at this size and springs back to 1. */
	UPROPERTY(Config, EditAnywhere, Category = "Motion")
	float PulseScale = 1.22f;

	UPROPERTY(Config, EditAnywhere, Category = "Motion", meta = (Units = "s"))
	float RippleSeconds = 0.8f;

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

	/** Status glow behind the board (Shaders/DI_Background.hlsl): its colour follows the ring fullness at this rate (1/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Glow")
	float MoodRate = 1.5f;

	/** A fusion blooms the glow by BloomFrom (a single merge) up to 1 (a long chain); the bloom fades at BloomFade (1/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Glow")
	float BloomFrom = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category = "Glow")
	float BloomFade = 1.6f;

	/** How fast the glow sinks to dusk at game over and comes back on a new run (1/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Glow")
	float OverRate = 1.2f;

	/** Tutorial hint: a breathing coral circle under what to tap, this many times its size, at this opacity. */
	UPROPERTY(Config, EditAnywhere, Category = "Tutorial")
	float HintScale = 1.4f;

	UPROPERTY(Config, EditAnywhere, Category = "Tutorial")
	float HintOpacity = 0.55f;

	/** How fast the hint fades in and out (1/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Tutorial")
	float HintFadeRate = 5.f;

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

	/** Moves become melody: cue steps land on the tones of the current chord, which moves on every MovesPerChord. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TArray<FDIChord> Progression;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = 1))
	int32 MovesPerChord = 6;

	/** Step 0 of the first octave, Hz (C5). */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float HarmonyRootHz = 523.2511f;

	/** Humanising: every note's volume varies by up to this share, and it starts up to this late. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float VolumeJitter = 0.1f;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (Units = "s"))
	float TimingJitter = 0.012f;

	/** Ambience (sea wash + calm pad, and a warm pad that grows as the ring fills) at full slider. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float SeaGain = 0.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float CalmPadGain = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float WarmPadGain = 0.14f;

	/** E4 G4 D5: added over the calm pad as the ring fills, a gentle lift rather than an alarm. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TArray<float> WarmPadHz = { 329.63f, 392.f, 587.33f };

	/** The ambience drops to this share while paused. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float PausedAmbience = 0.4f;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (Units = "s"))
	float AmbienceFadeSeconds = 1.5f;

	/** Slider defaults for a new player (0..1). */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float DefaultEffectsVolume = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	float DefaultAmbienceVolume = 0.4f;

	/** Moving stones stretch along their path: share of size per board unit/s, capped at StretchMax. */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float StretchPerSpeed = 0.00008f;

	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float StretchMax = 0.12f;

	/** An impact squashes the stone it hits by this share; stones within ImpactReachTiles wobble less with distance. */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float ImpactSquash = 0.14f;

	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float NeighbourSquash = 0.06f;

	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float ImpactReachTiles = 2.2f;

	/** The wobble after an impact: frequency (rad/s) and how fast it dies (1/s). */
	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float WobbleFreq = 14.f;

	UPROPERTY(Config, EditAnywhere, Category = "Body")
	float WobbleDecay = 5.f;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
