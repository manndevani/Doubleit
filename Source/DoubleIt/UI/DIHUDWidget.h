#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIHUDWidget.generated.h"

class ADoubleItGameMode;
class UBorder;
class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;
struct FDITheme;

/**
 * Whole in-game UI, built in C++ (no Widget Blueprint): score, best, pause button, tutorial line,
 * game-over line and the pause/settings panel. Layout units assume a 1080-wide reference (UMG DPI default).
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Build();
	UTextBlock* MakeText(float Size);
	UButton* MakeButton(UPanelWidget* Parent, TObjectPtr<UTextBlock>& OutLabel, float FontSize = 34.f);
	void ApplyTheme(const FDITheme& Theme);

	UFUNCTION() void HandlePause();
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleTutorial();
	UFUNCTION() void HandleSound();
	UFUNCTION() void HandleHaptics();
	UFUNCTION() void HandleTheme();

	TWeakObjectPtr<ADoubleItGameMode> GameMode;
	FLinearColor ScoreInk = FLinearColor::White;
	float CelebrateLeft = 0.f;

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

	bool bLastDark = false;
	bool bThemeApplied = false;
};
