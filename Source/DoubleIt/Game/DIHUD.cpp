#include "DIHUD.h"
#include "DoubleItGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(0x4A, 0x45, 0x40));
	const FLinearColor SoftInk = FLinearColor::FromSRGBColor(FColor(0x9A, 0x90, 0x86));
}

void ADIHUD::DrawHUD()
{
	Super::DrawHUD();

	const ADoubleItGameMode* GM = GetWorld()->GetAuthGameMode<ADoubleItGameMode>();
	if (!GM || !Canvas)
	{
		return;
	}

	const float H = Canvas->ClipY;
	const float S = Canvas->ClipX / 540.f;
	DrawCentered(LexToString(GM->GetScore()), H * 0.07f, 2.4f * S, Ink);
	DrawCentered(FString::Printf(TEXT("best %lld"), GM->GetBest()), H * 0.07f + 56.f * S, 1.1f * S, SoftInk);

	if (GM->IsGameOver())
	{
		DrawCentered(TEXT("a calm finish"), H * 0.80f, 1.6f * S, Ink);
		DrawCentered(TEXT("tap to begin again"), H * 0.80f + 44.f * S, 1.1f * S, SoftInk);
	}
}

void ADIHUD::DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	UFont* Font = GEngine->GetLargeFont();
	float W = 0.f, TextH = 0.f;
	GetTextSize(Text, W, TextH, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - W) * 0.5f, Y, Font, Scale);
}
