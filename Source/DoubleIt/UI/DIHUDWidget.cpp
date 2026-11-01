#include "DIHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/DIGameEvents.h"
#include "Game/DIText.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/DISaveGame.h"
#include "Game/DITheme.h"
#include "Game/DoubleItGameMode.h"
#include "Misc/Paths.h"

namespace
{
	constexpr float CelebrateSeconds = 1.2f;
	/** Fonts are staged as loose files (DirectoriesToAlwaysStageAsNonUFS) so Slate can load them on device. */
	FSlateFontInfo NunitoFont(float Size, bool bLight = false)
	{
		static const TSharedRef<const FCompositeFont> Regular = MakeShared<FStandaloneCompositeFont>(TEXT("Regular"),
			FPaths::ProjectContentDir() / TEXT("DoubleIt/Fonts/Nunito-400.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		// The variable font's default instance is ExtraLight: used only for the large, airy score.
		static const TSharedRef<const FCompositeFont> Light = MakeShared<FStandaloneCompositeFont>(TEXT("Light"),
			FPaths::ProjectContentDir() / TEXT("DoubleIt/Fonts/Nunito.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		return FSlateFontInfo(bLight ? Light : Regular, Size);
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Anchor, FVector2D Position, FVector2D Alignment = FVector2D(0.5, 0.5))
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetAutoSize(true);
		return Slot;
	}

	FText ThemeName(EDIThemeMode Mode)
	{
		switch (Mode)
		{
		case EDIThemeMode::Light: return DIText::Get(TEXT("Theme.Light"));
		case EDIThemeMode::Dark: return DIText::Get(TEXT("Theme.Dark"));
		default: return DIText::Get(TEXT("Theme.System"));
		}
	}

	FText OnOff(bool bOn) { return DIText::Get(bOn ? TEXT("HUD.On") : TEXT("HUD.Off")); }
}

TSharedRef<SWidget> UDIHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Build();
	}
	return Super::RebuildWidget();
}

UTextBlock* UDIHUDWidget::MakeText(float Size)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetFont(NunitoFont(Size));
	Text->SetJustification(ETextJustify::Center);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Text;
}

UButton* UDIHUDWidget::MakeButton(UPanelWidget* Parent, TObjectPtr<UTextBlock>& OutLabel, float FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	OutLabel = MakeText(FontSize);
	Button->AddChild(OutLabel);
	if (Parent)
	{
		Parent->AddChild(Button);
	}
	MenuButtons.Add(Button);
	MenuLabels.Add(OutLabel);
	return Button;
}

void UDIHUDWidget::Build()
{
	// Safe zone keeps the score and pause button clear of notches, punch holes and rounded corners.
	USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>();
	Safe->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Safe;
	Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Safe->AddChild(Root);

	Score = MakeText(104.f);
	Score->SetFont(NunitoFont(104.f, true));
	Place(Root, Score, FVector2D(0.5, 0.0), FVector2D(0, 140), FVector2D(0.5, 0.0));
	Best = MakeText(30.f);
	Place(Root, Best, FVector2D(0.5, 0.0), FVector2D(0, 300), FVector2D(0.5, 0.0));

	TutorialText = MakeText(36.f);
	TutorialText->SetAutoWrapText(true);
	// Stretches to 84% of the screen width, so it wraps instead of overflowing on narrow or small screens.
	UCanvasPanelSlot* TutSlot = Root->AddChildToCanvas(TutorialText);
	TutSlot->SetAnchors(FAnchors(0.08f, 0.f, 0.92f, 0.f));
	TutSlot->SetOffsets(FMargin(0, 380, 0, 160));
	TutorialText->SetJustification(ETextJustify::Center);

	GameOverTitle = MakeText(48.f);
	GameOverTitle->SetText(DIText::Get(TEXT("HUD.GameOverTitle")));
	Place(Root, GameOverTitle, FVector2D(0.5, 0.80), FVector2D::ZeroVector);
	GameOverHint = MakeText(30.f);
	GameOverHint->SetText(DIText::Get(TEXT("HUD.GameOverHint")));
	Place(Root, GameOverHint, FVector2D(0.5, 0.80), FVector2D(0, 80));

	PauseButton = MakeButton(nullptr, PauseGlyph, 56.f);
	PauseGlyph->SetText(FText::AsCultureInvariant(TEXT("II"))); // a glyph, not words
	Place(Root, PauseButton, FVector2D(0.0, 0.0), FVector2D(48, 72), FVector2D(0.0, 0.0));
	PauseButton->OnClicked.AddDynamic(this, &UDIHUDWidget::HandlePause);

	PausePanel = WidgetTree->ConstructWidget<UBorder>();
	PausePanel->SetHorizontalAlignment(HAlign_Center);
	PausePanel->SetVerticalAlignment(VAlign_Center);
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PausePanel);
	PanelSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	PanelSlot->SetOffsets(FMargin(0));

	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>();
	PausePanel->SetContent(Menu);
	PauseTitle = MakeText(64.f);
	PauseTitle->SetText(DIText::Get(TEXT("HUD.Paused")));
	Menu->AddChildToVerticalBox(PauseTitle)->SetPadding(FMargin(0, 0, 0, 48));

	TObjectPtr<UTextBlock> Label;
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleResume);
	Label->SetText(DIText::Get(TEXT("HUD.Resume")));
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleRestart);
	Label->SetText(DIText::Get(TEXT("HUD.NewGame")));
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleTutorial);
	Label->SetText(DIText::Get(TEXT("HUD.HowToPlay")));
	MakeButton(Menu, SoundLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleSound);
	MakeButton(Menu, HapticsLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleHaptics);
	MakeButton(Menu, ThemeLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleTheme);

	for (UWidget* Child : Menu->GetAllChildren())
	{
		if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(Child->Slot))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			if (Child != PauseTitle)
			{
				BoxSlot->SetPadding(FMargin(0, 12));
			}
		}
	}
}

void UDIHUDWidget::Bind(ADoubleItGameMode* InGameMode)
{
	GameMode = InGameMode;
	if (UDIGameEvents* Events = UDIGameEvents::Get(InGameMode))
	{
		Events->StateChanged.AddUObject(this, &UDIHUDWidget::Refresh);
		Events->NewBest.AddUObject(this, &UDIHUDWidget::CelebrateBest);
	}
}

void UDIHUDWidget::CelebrateBest()
{
	CelebrateLeft = CelebrateSeconds;
}

void UDIHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (CelebrateLeft <= 0.f || !Score)
	{
		return;
	}
	CelebrateLeft = FMath::Max(CelebrateLeft - InDeltaTime, 0.f);
	const float T = 1.f - CelebrateLeft / CelebrateSeconds;     // 0 -> 1
	const float Swell = FMath::Sin(FMath::Min(T * 3.f, 1.f) * PI); // quick swell in the first third
	const float Glow = FMath::Square(1.f - T);                      // gold fades back to ink
	Score->SetRenderScale(FVector2D(1.f + 0.12f * Swell));
	Score->SetColorAndOpacity(FSlateColor(FMath::Lerp(ScoreInk, FLinearColor(FColor(0xFF, 0xD4, 0x80)), Glow)));
}

void UDIHUDWidget::ApplyTheme(const FDITheme& Theme)
{
	ScoreInk = Theme.Ink;
	for (UTextBlock* Text : { Score.Get(), TutorialText.Get(), GameOverTitle.Get(), PauseTitle.Get() })
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.Ink));
	}
	for (UTextBlock* Text : { Best.Get(), GameOverHint.Get() })
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.SoftInk));
	}
	for (UTextBlock* Text : MenuLabels)
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.Ink));
	}

	FButtonStyle Style;
	Style.SetNormal(FSlateRoundedBoxBrush(Theme.Button, 48.f));
	Style.SetHovered(FSlateRoundedBoxBrush(Theme.Button, 48.f));
	Style.SetPressed(FSlateRoundedBoxBrush(Theme.ButtonPressed, 48.f));
	Style.SetNormalPadding(FMargin(64, 22));
	Style.SetPressedPadding(FMargin(64, 22));
	for (UButton* Button : MenuButtons)
	{
		Button->SetStyle(Style);
	}

	FButtonStyle Ghost = Style;
	Ghost.SetNormal(FSlateNoResource());
	Ghost.SetHovered(FSlateNoResource());
	Ghost.SetPressed(FSlateRoundedBoxBrush(Theme.ButtonPressed, 48.f));
	Ghost.SetNormalPadding(FMargin(28, 12));
	Ghost.SetPressedPadding(FMargin(28, 12));
	PauseButton->SetStyle(Ghost);
	PauseGlyph->SetColorAndOpacity(FSlateColor(Theme.SoftInk));

	PausePanel->SetBrushColor(Theme.Panel);
}

void UDIHUDWidget::Refresh()
{
	ADoubleItGameMode* GM = GameMode.Get();
	if (!GM || !Root)
	{
		return;
	}

	const bool bDark = GM->IsDarkTheme();
	if (!bThemeApplied || bDark != bLastDark)
	{
		ApplyTheme(FDITheme::Get(bDark));
		bLastDark = bDark;
		bThemeApplied = true;
	}

	const bool bTutorial = GM->IsInTutorial();
	Score->SetText(bTutorial ? GM->GetTutorialProgress() : DIText::Number(GM->GetScore()));
	Best->SetText(bTutorial ? FText::GetEmpty() : FText::Format(DIText::Get(TEXT("HUD.Best")), DIText::Number(GM->GetBest())));
	TutorialText->SetText(bTutorial ? GM->GetTutorialText() : FText::GetEmpty());

	const ESlateVisibility Over = GM->IsGameOver() && !bTutorial ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	GameOverTitle->SetVisibility(Over);
	GameOverHint->SetVisibility(Over);

	const bool bPaused = GM->IsBoardPaused();
	PausePanel->SetVisibility(bPaused ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PauseButton->SetVisibility(bPaused ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (const UDISaveGame* Save = GM->GetSave())
	{
		SoundLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Sound")), OnOff(Save->bSound)));
		HapticsLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Vibration")), OnOff(Save->bHaptics)));
		ThemeLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Theme")), ThemeName(Save->ThemeMode)));
	}
}

void UDIHUDWidget::HandlePause() { if (GameMode.IsValid()) GameMode->SetBoardPaused(true); }
void UDIHUDWidget::HandleResume() { if (GameMode.IsValid()) GameMode->SetBoardPaused(false); }
void UDIHUDWidget::HandleRestart() { if (GameMode.IsValid()) GameMode->RestartRun(); }
void UDIHUDWidget::HandleTutorial() { if (GameMode.IsValid()) GameMode->StartTutorial(); }
void UDIHUDWidget::HandleSound() { if (GameMode.IsValid()) GameMode->ToggleSound(); }
void UDIHUDWidget::HandleHaptics() { if (GameMode.IsValid()) GameMode->ToggleHaptics(); }
void UDIHUDWidget::HandleTheme() { if (GameMode.IsValid()) GameMode->CycleTheme(); }
