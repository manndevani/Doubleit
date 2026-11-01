#include "DIHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Game/DIGameEvents.h"
#include "Game/DIText.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/DISaveGame.h"
#include "Game/DITheme.h"
#include "Game/DoubleItGameMode.h"
#include "Game/DIGameSettings.h"
#include "Game/DIShop.h"
#include "Core/DIGame.h"
#include "Misc/Paths.h"

namespace
{
	constexpr float CelebrateSeconds = 1.2f;
	constexpr float NudgeSeconds = 0.5f;
	constexpr float NudgeSwell = 0.07f;
	constexpr float SliderStep = 0.05f;
	constexpr float SliderBar = 10.f, SliderKnob = 44.f; // thick enough for a thumb
	constexpr float BreezeDotSize = 36.f;
	constexpr float BreezeHoldScale = 1.25f;  // the dot while its breeze is in the centre
	constexpr float BreezePulseSpeed = 3.f;   // rad/s, while the tutorial asks for the button
	constexpr float ToastFadeSeconds = 0.6f;
	constexpr float ShopTextWidth = 820.f;
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

	// Breeze button, top right opposite pause: a pearl dot and how many you own. Tap: bring one in, or put it back.
	BreezeButton = WidgetTree->ConstructWidget<UButton>();
	UHorizontalBox* BreezeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	BreezeButton->AddChild(BreezeRow);
	USizeBox* DotBox = WidgetTree->ConstructWidget<USizeBox>();
	DotBox->SetWidthOverride(BreezeDotSize);
	DotBox->SetHeightOverride(BreezeDotSize);
	BreezeDot = WidgetTree->ConstructWidget<UBorder>();
	BreezeDot->SetVisibility(ESlateVisibility::HitTestInvisible);
	DotBox->AddChild(BreezeDot);
	BreezeRow->AddChildToHorizontalBox(DotBox)->SetVerticalAlignment(VAlign_Center);
	BreezeCount = MakeText(44.f);
	UHorizontalBoxSlot* CountSlot = BreezeRow->AddChildToHorizontalBox(BreezeCount);
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetPadding(FMargin(16, 0, 0, 0));
	Place(Root, BreezeButton, FVector2D(1.0, 0.0), FVector2D(-48, 72), FVector2D(1.0, 0.0));
	BreezeButton->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleBreeze);

	ToastText = MakeText(30.f);
	Place(Root, ToastText, FVector2D(0.5, 0.0), FVector2D(0, 344), FVector2D(0.5, 0.0));
	ToastText->SetVisibility(ESlateVisibility::Collapsed);

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
	MakeButton(Menu, Label)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleShop);
	Label->SetText(DIText::Get(TEXT("HUD.Breezes")));
	MakeButton(Menu, SoundLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleSound);
	EffectsSlider = MakeVolume(Menu, TEXT("HUD.Effects"));
	EffectsSlider->OnValueChanged.AddDynamic(this, &UDIHUDWidget::HandleEffects);
	EffectsSlider->OnMouseCaptureEnd.AddDynamic(this, &UDIHUDWidget::HandleEffectsReleased);
	EffectsSlider->OnControllerCaptureEnd.AddDynamic(this, &UDIHUDWidget::HandleEffectsReleased);
	AmbienceSlider = MakeVolume(Menu, TEXT("HUD.Ambience"));
	AmbienceSlider->OnValueChanged.AddDynamic(this, &UDIHUDWidget::HandleAmbience);
	AmbienceSlider->OnMouseCaptureEnd.AddDynamic(this, &UDIHUDWidget::HandleAmbienceReleased);
	AmbienceSlider->OnControllerCaptureEnd.AddDynamic(this, &UDIHUDWidget::HandleAmbienceReleased);
	MakeButton(Menu, HapticsLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleHaptics);
	MakeButton(Menu, ThemeLabel)->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleTheme);

	for (UWidget* Child : Menu->GetAllChildren())
	{
		if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(Child->Slot))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			if (SliderLabels.Contains(Child))
			{
				BoxSlot->SetPadding(FMargin(0, 16, 0, 0)); // a label sits right on top of its slider
			}
			else if (Child != PauseTitle)
			{
				BoxSlot->SetPadding(FMargin(0, 12));
			}
		}
	}

	// Shop: what a breeze does, how many you have, the packs the store sells (filled in RefreshShop), close.
	ShopPanel = WidgetTree->ConstructWidget<UBorder>();
	ShopPanel->SetHorizontalAlignment(HAlign_Center);
	ShopPanel->SetVerticalAlignment(VAlign_Center);
	UCanvasPanelSlot* ShopSlot = Root->AddChildToCanvas(ShopPanel);
	ShopSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	ShopSlot->SetOffsets(FMargin(0));
	UVerticalBox* ShopBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ShopPanel->SetContent(ShopBox);

	UTextBlock* ShopTitle = MakeText(64.f);
	ShopTitle->SetText(DIText::Get(TEXT("Shop.Title")));
	ShopBox->AddChildToVerticalBox(ShopTitle)->SetPadding(FMargin(0, 0, 0, 24));
	PanelTexts.Add(ShopTitle);
	USizeBox* BodyBox = WidgetTree->ConstructWidget<USizeBox>();
	BodyBox->SetWidthOverride(ShopTextWidth);
	ShopBody = MakeText(30.f);
	ShopBody->SetAutoWrapText(true);
	BodyBox->AddChild(ShopBody);
	ShopBox->AddChildToVerticalBox(BodyBox)->SetHorizontalAlignment(HAlign_Center);
	PanelSoftTexts.Add(ShopBody);
	ShopHave = MakeText(40.f);
	ShopBox->AddChildToVerticalBox(ShopHave)->SetPadding(FMargin(0, 32, 0, 16));
	PanelTexts.Add(ShopHave);
	ShopPacks = WidgetTree->ConstructWidget<UVerticalBox>();
	ShopBox->AddChildToVerticalBox(ShopPacks)->SetHorizontalAlignment(HAlign_Fill);
	ShopClosed = MakeText(30.f);
	ShopClosed->SetText(DIText::Get(TEXT("Shop.Closed")));
	ShopBox->AddChildToVerticalBox(ShopClosed)->SetPadding(FMargin(0, 12));
	PanelSoftTexts.Add(ShopClosed);
	UButton* Close = MakeButton(ShopBox, Label);
	Close->OnClicked.AddDynamic(this, &UDIHUDWidget::HandleShopClose);
	Label->SetText(DIText::Get(TEXT("Shop.Close")));
	if (UVerticalBoxSlot* CloseSlot = Cast<UVerticalBoxSlot>(Close->Slot))
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Fill);
		CloseSlot->SetPadding(FMargin(0, 24, 0, 0));
	}
}

void UDIPackButton::HandleClicked()
{
	if (UDIShop* S = Shop.Get())
	{
		S->Buy(ProductId);
	}
}

void UDIHUDWidget::Bind(ADoubleItGameMode* InGameMode)
{
	GameMode = InGameMode;
	if (UDIGameEvents* Events = UDIGameEvents::Get(InGameMode))
	{
		Events->StateChanged.AddUObject(this, &UDIHUDWidget::Refresh);
		Events->NewBest.AddUObject(this, &UDIHUDWidget::CelebrateBest);
		Events->TutorialNudge.AddUObject(this, &UDIHUDWidget::Nudge);
		Events->PieceEarned.AddWeakLambda(this, [this](int64 Value)
		{
			Toast(FText::Format(DIText::Get(TEXT("HUD.BreezeEarned")), DIText::Number(Value)));
		});
		Events->PiecesBought.AddWeakLambda(this, [this](int32 Count)
		{
			Toast(FText::Format(DIText::Get(TEXT("Shop.Bought")), FText::AsNumber(Count)));
		});
	}
}

void UDIHUDWidget::Toast(const FText& Text)
{
	ToastText->SetText(Text);
	ToastText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ToastLeft = UDIGameSettings::Get().ToastSeconds;
}

void UDIHUDWidget::CelebrateBest()
{
	CelebrateLeft = CelebrateSeconds;
}

void UDIHUDWidget::Nudge()
{
	NudgeLeft = NudgeSeconds;
}

void UDIHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (NudgeLeft > 0.f && TutorialText)
	{
		NudgeLeft = FMath::Max(NudgeLeft - InDeltaTime, 0.f);
		TutorialText->SetRenderScale(FVector2D(1.f + NudgeSwell * FMath::Sin((1.f - NudgeLeft / NudgeSeconds) * PI)));
	}
	if (ToastLeft > 0.f && ToastText)
	{
		ToastLeft = FMath::Max(ToastLeft - InDeltaTime, 0.f);
		ToastText->SetRenderOpacity(FMath::Min(ToastLeft / ToastFadeSeconds, 1.f));
		if (ToastLeft <= 0.f)
		{
			ToastText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (const ADoubleItGameMode* GM = GameMode.Get(); GM && BreezeDot)
	{
		// The dot grows while its breeze is in the centre, and breathes while the tutorial asks for it.
		PulseTime = GM->WantsBringIn() ? PulseTime + InDeltaTime : 0.f;
		const float Pulse = UDIGameSettings::Get().ButtonPulse * 0.5f * (1.f - FMath::Cos(PulseTime * BreezePulseSpeed));
		BreezeDot->SetRenderScale(FVector2D((GM->IsHolding() ? BreezeHoldScale : 1.f) + Pulse));
	}
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
	BreezeButton->SetStyle(Ghost);
	BreezeDot->SetBrush(FSlateRoundedBoxBrush(Theme.Breeze, BreezeDotSize * 0.5f, Theme.SoftInk, 2.f));
	BreezeCount->SetColorAndOpacity(FSlateColor(Theme.SoftInk));
	ToastText->SetColorAndOpacity(FSlateColor(Theme.SoftInk));

	PausePanel->SetBrushColor(Theme.Panel);
	ShopPanel->SetBrushColor(Theme.Panel);
	for (UTextBlock* Text : PanelTexts)
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.Ink));
	}
	for (UTextBlock* Text : PanelSoftTexts)
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.SoftInk));
	}

	FSliderStyle Bar;
	Bar.SetNormalBarImage(FSlateRoundedBoxBrush(Theme.Button, SliderBar * 0.5f));
	Bar.SetHoveredBarImage(FSlateRoundedBoxBrush(Theme.Button, SliderBar * 0.5f));
	Bar.SetDisabledBarImage(FSlateRoundedBoxBrush(Theme.Button, SliderBar * 0.5f));
	Bar.SetBarThickness(SliderBar);
	const FSlateRoundedBoxBrush Knob(Theme.Ink, SliderKnob * 0.5f, FVector2f(SliderKnob));
	Bar.SetNormalThumbImage(Knob);
	Bar.SetHoveredThumbImage(Knob);
	Bar.SetDisabledThumbImage(Knob);
	for (USlider* Slider : { EffectsSlider.Get(), AmbienceSlider.Get() })
	{
		Slider->SetWidgetStyle(Bar);
	}
	for (UTextBlock* Text : SliderLabels)
	{
		Text->SetColorAndOpacity(FSlateColor(Theme.SoftInk));
	}
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
	bShopOpen &= bPaused;
	PausePanel->SetVisibility(bPaused && !bShopOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ShopPanel->SetVisibility(bShopOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PauseButton->SetVisibility(bPaused ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	// The tutorial shows the button only in the step that teaches it, free (no count).
	const bool bBreeze = !bPaused && (bTutorial ? GM->OffersBringIn() : !GM->IsGameOver());
	BreezeButton->SetVisibility(bBreeze ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BreezeCount->SetText(bTutorial ? FText::GetEmpty() : FText::AsNumber(GM->GetOwnedCount()));
	if (bShopOpen)
	{
		RefreshShop(*GM);
	}

	if (const UDISaveGame* Save = GM->GetSave())
	{
		SoundLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Sound")), OnOff(Save->bSound)));
		HapticsLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Vibration")), OnOff(Save->bHaptics)));
		ThemeLabel->SetText(FText::Format(DIText::Get(TEXT("HUD.Theme")), ThemeName(Save->ThemeMode)));
		EffectsSlider->SetValue(Save->EffectsVolume);
		AmbienceSlider->SetValue(Save->AmbienceVolume);
	}
}

void UDIHUDWidget::HandlePause() { if (GameMode.IsValid()) GameMode->SetBoardPaused(true); }
void UDIHUDWidget::HandleResume() { if (GameMode.IsValid()) GameMode->SetBoardPaused(false); }
void UDIHUDWidget::HandleRestart() { if (GameMode.IsValid()) GameMode->RestartRun(); }
void UDIHUDWidget::HandleTutorial() { if (GameMode.IsValid()) GameMode->StartTutorial(); }
void UDIHUDWidget::HandleSound() { if (GameMode.IsValid()) GameMode->ToggleSound(); }
void UDIHUDWidget::HandleHaptics() { if (GameMode.IsValid()) GameMode->ToggleHaptics(); }
void UDIHUDWidget::HandleTheme() { if (GameMode.IsValid()) GameMode->CycleTheme(); }
void UDIHUDWidget::HandleEffects(float Value) { if (GameMode.IsValid()) GameMode->SetEffectsVolume(Value, false); }
void UDIHUDWidget::HandleAmbience(float Value) { if (GameMode.IsValid()) GameMode->SetAmbienceVolume(Value, false); }

// Live while dragging; saved once when the finger lifts (effects also play a sample at the new level).
void UDIHUDWidget::HandleEffectsReleased() { if (GameMode.IsValid()) GameMode->SetEffectsVolume(EffectsSlider->GetValue(), true); }
void UDIHUDWidget::HandleAmbienceReleased() { if (GameMode.IsValid()) GameMode->SetAmbienceVolume(AmbienceSlider->GetValue(), true); }

USlider* UDIHUDWidget::MakeVolume(UVerticalBox* Menu, const TCHAR* LabelKey)
{
	UTextBlock* Label = MakeText(28.f);
	Label->SetText(DIText::Get(LabelKey));
	Menu->AddChildToVerticalBox(Label);
	SliderLabels.Add(Label);

	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetStepSize(SliderStep);
	Slider->SetMinValue(0.f);
	Slider->SetMaxValue(1.f);
	Slider->MouseUsesStep = true;
	Menu->AddChildToVerticalBox(Slider);
	return Slider;
}

void UDIHUDWidget::HandleBreeze()
{
	ADoubleItGameMode* GM = GameMode.Get();
	if (!GM)
	{
		return;
	}
	// None left (and none in the centre): the button opens the shop instead.
	if (!GM->OffersBringIn() && !GM->IsInTutorial() && !GM->IsGameOver())
	{
		OpenShop();
		return;
	}
	GM->ToggleBringIn();
}

void UDIHUDWidget::HandleShop() { OpenShop(); }

void UDIHUDWidget::OpenShop()
{
	if (ADoubleItGameMode* GM = GameMode.Get())
	{
		bShopFromPause = GM->IsBoardPaused();
		bShopOpen = true;
		GM->SetBoardPaused(true);
		Refresh();
	}
}

void UDIHUDWidget::HandleShopClose()
{
	bShopOpen = false;
	if (ADoubleItGameMode* GM = GameMode.Get())
	{
		if (bShopFromPause)
		{
			Refresh(); // back to the pause menu it came from
		}
		else
		{
			GM->SetBoardPaused(false);
		}
	}
}

void UDIHUDWidget::RefreshShop(const ADoubleItGameMode& GM)
{
	const FDIGame* Game = GM.GetGame();
	ShopBody->SetText(FText::Format(DIText::Get(TEXT("Shop.Body")), DIText::Number(Game ? Game->GetMode().Config.RewardFrom : 0)));
	ShopHave->SetText(FText::Format(DIText::Get(TEXT("Shop.Have")), FText::AsNumber(GM.GetOwnedCount())));

	static const TArray<FDIStoreProduct> None;
	UDIShop* Shop = GM.GetShop();
	const TArray<FDIStoreProduct>& Products = Shop ? Shop->GetProducts() : None;
	ShopClosed->SetVisibility(Products.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (PackButtons.Num() == Products.Num())
	{
		return;
	}

	// The store answered with a different list: rebuild one button per pack it sells.
	for (UWidget* Old : ShopPacks->GetAllChildren())
	{
		if (UButton* OldButton = Cast<UButton>(Old))
		{
			MenuLabels.Remove(Cast<UTextBlock>(OldButton->GetContent()));
			MenuButtons.Remove(OldButton);
		}
	}
	ShopPacks->ClearChildren();
	PackButtons.Reset();
	for (const FDIStoreProduct& Product : Products)
	{
		const FDIPiecePack* Pack = UDIGameSettings::Get().Packs.FindByPredicate([&](const FDIPiecePack& P) { return P.ProductId == Product.Id; });
		if (!Pack)
		{
			continue;
		}
		TObjectPtr<UTextBlock> Label;
		UButton* Button = MakeButton(ShopPacks, Label);
		Label->SetText(FText::Format(DIText::Get(TEXT("Shop.Pack")), FText::AsNumber(Pack->Count), Product.Price));
		if (UVerticalBoxSlot* PackSlot = Cast<UVerticalBoxSlot>(Button->Slot))
		{
			PackSlot->SetHorizontalAlignment(HAlign_Fill);
			PackSlot->SetPadding(FMargin(0, 12));
		}
		UDIPackButton* Proxy = NewObject<UDIPackButton>(this);
		Proxy->ProductId = Product.Id;
		Proxy->Shop = Shop;
		Button->OnClicked.AddDynamic(Proxy, &UDIPackButton::HandleClicked);
		PackButtons.Add(Proxy);
	}
	ApplyTheme(FDITheme::Get(bLastDark)); // style the new buttons
}
