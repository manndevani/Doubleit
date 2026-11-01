#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIHUDWidget.generated.h"

class ADoubleItGameMode;
class UBorder;
class UButton;
class UCanvasPanel;
class UPanelWidget;
class USlider;
class UTextBlock;
class UVerticalBox;
struct FDITheme;
class UDIShop;

/** One shop pack button: remembers its product, so every button can share one click handler shape. */
UCLASS()
class UDIPackButton : public UObject
{
	GENERATED_BODY()

public:
	FName ProductId;
	TWeakObjectPtr<UDIShop> Shop;
	UFUNCTION() void HandleClicked();
};

/**
 * Whole in-game UI, built in C++ (no Widget Blueprint): score, best, pause button, tutorial line,
 * game-over line, the pause/settings panel, the breeze button (top right) and the shop panel. Layout units assume a 1080-wide reference (UMG DPI default).
 */
UCLASS()
class DOUBLEIT_API UDIHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Reads state from the game mode and refreshes on its events (UDIGameEvents). */
	void Bind(ADoubleItGameMode* InGameMode);

	/** Pulls everything from the game mode (score, labels, visibility, theme). */
	void Refresh();

	/** Passing your best mid-run: the score swells and glows sun-gold, then settles back (~1.2 s). */
	void CelebrateBest();

	/** The tutorial refused a tap: the instruction swells once, pointing back at what to do. */
	void Nudge();

	/** A short line under the best score that fades out (pieces earned or bought). */
	void Toast(const FText& Text);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Build();
	UTextBlock* MakeText(float Size);
	UButton* MakeButton(UPanelWidget* Parent, TObjectPtr<UTextBlock>& OutLabel, float FontSize = 34.f);
	void ApplyTheme(const FDITheme& Theme);
	USlider* MakeVolume(UVerticalBox* Menu, const TCHAR* LabelKey);

	UFUNCTION() void HandlePause();
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleTutorial();
	UFUNCTION() void HandleSound();
	UFUNCTION() void HandleHaptics();
	UFUNCTION() void HandleTheme();
	UFUNCTION() void HandleEffects(float Value);
	UFUNCTION() void HandleAmbience(float Value);
	UFUNCTION() void HandleEffectsReleased();
	UFUNCTION() void HandleAmbienceReleased();
	UFUNCTION() void HandleBreeze();
	UFUNCTION() void HandleShop();
	UFUNCTION() void HandleSupport();
	UFUNCTION() void HandleAdsToggle();
	UFUNCTION() void HandlePrivacy();
	UFUNCTION() void HandlePanelClose();
	/** Shows Panel over the board (pausing it); closing returns to the pause menu if it came from there. */
	void OpenPanel(UBorder* Panel);
	UVerticalBox* MakePanel(TObjectPtr<UBorder>& OutPanel, const TCHAR* TitleKey);
	UTextBlock* MakePanelBody(UVerticalBox* Box);
	void AddPanelClose(UVerticalBox* Box);
	void RefreshShop(const ADoubleItGameMode& GM);

	TWeakObjectPtr<ADoubleItGameMode> GameMode;
	FLinearColor ScoreInk = FLinearColor::White;
	float CelebrateLeft = 0.f;
	float NudgeLeft = 0.f;
	float ToastLeft = 0.f;
	float PulseTime = 0.f;
	bool bPanelFromPause = false;

	UPROPERTY() TObjectPtr<UCanvasPanel> Root;
	UPROPERTY() TObjectPtr<UTextBlock> Score;
	UPROPERTY() TObjectPtr<UTextBlock> Best;
	UPROPERTY() TObjectPtr<UTextBlock> TutorialText;
	UPROPERTY() TObjectPtr<UTextBlock> GameOverTitle;
	UPROPERTY() TObjectPtr<UTextBlock> GameOverHint;
	UPROPERTY() TObjectPtr<UButton> PauseButton;
	UPROPERTY() TObjectPtr<UTextBlock> PauseGlyph;
	UPROPERTY() TObjectPtr<UBorder> PausePanel;
	UPROPERTY() TObjectPtr<UTextBlock> PauseTitle;
	UPROPERTY() TObjectPtr<UTextBlock> SoundLabel;
	UPROPERTY() TObjectPtr<UTextBlock> HapticsLabel;
	UPROPERTY() TObjectPtr<UTextBlock> ThemeLabel;
	UPROPERTY() TArray<TObjectPtr<UButton>> MenuButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> MenuLabels;
	UPROPERTY() TObjectPtr<USlider> EffectsSlider;
	UPROPERTY() TObjectPtr<USlider> AmbienceSlider;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SliderLabels;
	UPROPERTY() TObjectPtr<UButton> BreezeButton;
	UPROPERTY() TObjectPtr<UBorder> BreezeDot;
	UPROPERTY() TObjectPtr<UTextBlock> BreezeCount;
	UPROPERTY() TObjectPtr<UTextBlock> ToastText;
	UPROPERTY() TObjectPtr<UBorder> ShopPanel;
	UPROPERTY() TObjectPtr<UBorder> SupportPanel;
	UPROPERTY() TObjectPtr<UBorder> ActivePanel; // the shop or support panel on show, if any
	UPROPERTY() TArray<TObjectPtr<UBorder>> Panels;
	UPROPERTY() TObjectPtr<UTextBlock> AdsLabel;
	UPROPERTY() TObjectPtr<UButton> PrivacyButton;
	UPROPERTY() TObjectPtr<UBorder> AdPreview;
	UPROPERTY() TObjectPtr<UTextBlock> AdPreviewText;
	UPROPERTY() TObjectPtr<UWidget> AdPreviewBox;
	UPROPERTY() TObjectPtr<UTextBlock> ShopBody;
	UPROPERTY() TObjectPtr<UTextBlock> ShopHave;
	UPROPERTY() TObjectPtr<UTextBlock> ShopClosed;
	UPROPERTY() TObjectPtr<UVerticalBox> ShopPacks;
	UPROPERTY() TArray<TObjectPtr<UDIPackButton>> PackButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PanelTexts;     // shop title and count, themed as ink
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PanelSoftTexts; // shop explanation and notes, themed as soft ink

	bool bLastDark = false;
	bool bThemeApplied = false;
};
