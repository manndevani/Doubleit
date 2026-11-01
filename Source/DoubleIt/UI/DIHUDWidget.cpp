#include "DIHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/DISaveGame.h"
#include "Game/DITheme.h"
#include "Game/DoubleItGameMode.h"
#include "Misc/Paths.h"

namespace
{
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

	const TCHAR* ThemeName(EDIThemeMode Mode)
	{
		switch (Mode)
		{
		case EDIThemeMode::Light: return TEXT("light");
		case EDIThemeMode::Dark: return TEXT("dark");
		default: return TEXT("follow phone");
		}
	}
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
	Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	Score = MakeText(104.f);
	Score->SetFont(NunitoFont(104.f, true));
	Place(Root, Score, FVector2D(0.5, 0.0), FVector2D(0, 140), FVector2D(0.5, 0.0));
	Best = MakeText(30.f);
	Place(Root, Best, FVector2D(0.5, 0.0), FVector2D(0, 300), FVector2D(0.5, 0.0));

	TutorialText = MakeText(36.f);
	TutorialText->SetAutoWrapText(true);
	UCanvasPanelSlot* TutSlot = Place(Root, TutorialText, FVector2D(0.5, 0.0), FVector2D(0, 380), FVector2D(0.5, 0.0));
	TutSlot->SetAutoSize(false);
	TutSlot->SetSize(FVector2D(900, 160));

	GameOverTitle = MakeText(48.f);
	GameOverTitle->SetText(FText::FromString(TEXT("a calm finish")));
	Place(Root, GameOverTitle, FVector2D(0.5, 0.80), FVector2D::ZeroVector);
	GameOverHint = MakeText(30.f);
	GameOverHint->SetText(FText::FromString(TEXT("tap to begin again")));
	Place(Root, GameOverHint, FVector2D(0.5, 0.80), FVector2D(0, 80));

	PauseButton = MakeButton(nullptr, PauseGlyph, 56.f);
	PauseGlyph->SetText(FText::FromString(TEXT("II")));
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
	PauseTitle->SetText(FText::FromString(TEXT("paused")));
	Menu->AddChildToVerticalBox(PauseTitle)->SetPadding(FMargin(0, 0, 0, 48));

	TObjectPtr<UTextBlock> Label;
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleResume);
	Label->SetText(FText::FromString(TEXT("resume")));
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleRestart);
	Label->SetText(FText::FromString(TEXT("new game")));
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleTutorial);
	Label->SetText(FText::FromString(TEXT("how to play")));
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

void UDIHUDWidget::ApplyTheme(const FDITheme& Theme)
{
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
	Score->SetText(FText::FromString(bTutorial ? GM->GetTutorialProgress() : LexToString(GM->GetScore())));
	Best->SetText(FText::FromString(bTutorial ? FString() : FString::Printf(TEXT("best %lld"), GM->GetBest())));
	TutorialText->SetText(FText::FromString(bTutorial ? GM->GetTutorialText() : FString()));

	const ESlateVisibility Over = GM->IsGameOver() && !bTutorial ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	GameOverTitle->SetVisibility(Over);
	GameOverHint->SetVisibility(Over);

	const bool bPaused = GM->IsBoardPaused();
	PausePanel->SetVisibility(bPaused ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PauseButton->SetVisibility(bPaused ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (const UDISaveGame* Save = GM->GetSave())
	{
		SoundLabel->SetText(FText::FromString(FString::Printf(TEXT("sound: %s"), Save->bSound ? TEXT("on") : TEXT("off"))));
		HapticsLabel->SetText(FText::FromString(FString::Printf(TEXT("vibration: %s"), Save->bHaptics ? TEXT("on") : TEXT("off"))));
		ThemeLabel->SetText(FText::FromString(FString::Printf(TEXT("theme: %s"), ThemeName(Save->ThemeMode))));
	}
}

void UDIHUDWidget::HandlePause() { if (GameMode.IsValid()) GameMode->SetBoardPaused(true); }
void UDIHUDWidget::HandleResume() { if (GameMode.IsValid()) GameMode->SetBoardPaused(false); }
void UDIHUDWidget::HandleRestart() { if (GameMode.IsValid()) GameMode->RestartRun(); }
void UDIHUDWidget::HandleTutorial() { if (GameMode.IsValid()) GameMode->StartTutorial(); }
void UDIHUDWidget::HandleSound() { if (GameMode.IsValid()) GameMode->ToggleSound(); }
void UDIHUDWidget::HandleHaptics() { if (GameMode.IsValid()) GameMode->ToggleHaptics(); }
void UDIHUDWidget::HandleTheme() { if (GameMode.IsValid()) GameMode->CycleTheme(); }
