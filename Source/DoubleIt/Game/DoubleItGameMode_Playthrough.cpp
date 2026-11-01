// Full-run automation: a bot plays one Classic game through real screen taps and reports feel metrics.
// Run via Scripts/playthrough.ps1. Never compiled into Shipping.
#include "DoubleItGameMode.h"

#if !UE_BUILD_SHIPPING
#include "DoubleIt.h"
#include "DIPlayerController.h"
#include "DIRingActor.h"
#include "Core/DIPieceType.h"
#include "Core/DIRingLayout.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace
{
	FVector2D AtAngle(float Angle, float Radius = DIRingLayout::RingRadius)
	{
		return FVector2D(Radius * FMath::Sin(Angle), Radius * FMath::Cos(Angle));
	}

	FVector2D SlotPoint(int32 Slot, int32 Count, float Offset)
	{
		if (Count == 0)
		{
			return AtAngle(Offset);
		}
		// Slot 0 and slot Count are the same gap on a circle (between the last and first tile).
		return AtAngle(DIRingLayout::TileAngle(Slot == 0 ? Count : Slot, Count, Offset) - UE_PI / Count);
	}

	/** Greedy value of placing the current centre in the best slot. */
	int64 BestPlacement(const FDIGame& Game, int32& OutSlot)
	{
		const int32 Num = Game.GetRing().Num();
		int64 Best = TNumericLimits<int64>::Lowest();
		OutSlot = Num;
		for (int32 Slot = 1; Slot <= Num; ++Slot)
		{
			FDIGame Trial = Game;
			Trial.PlaceAt(Slot);
			const int64 Gain = (Trial.GetScore() - Game.GetScore()) * 4 - Trial.GetRing().Num() - (Trial.IsGameOver() ? 1000 : 0);
			if (Gain > Best)
			{
				Best = Gain;
				OutSlot = Slot;
			}
		}
		return Best;
	}

	bool SameBoard(const FDIGame& A, const FDIGame& B)
	{
		if (!(A.GetCenter() == B.GetCenter()) || A.GetRing().Num() != B.GetRing().Num() || A.GetScore() != B.GetScore())
		{
			return false;
		}
		for (int32 i = 0; i < A.GetRing().Num(); ++i)
		{
			if (!(A.GetRing()[i] == B.GetRing()[i]))
			{
				return false;
			}
		}
		return true;
	}
}

void ADoubleItGameMode::PlaythroughSchedule(float Delay)
{
	GetWorldTimerManager().SetTimer(PlaythroughTimer, this, &ADoubleItGameMode::PlaythroughStep, FMath::Max(Delay, 0.01f), false);
}

void ADoubleItGameMode::PlaythroughShot(const FString& Label, float Delay)
{
	if (PlaythroughDir.IsEmpty() || Pt.Shots >= 16)
	{
		return;
	}
	const FString Path = PlaythroughDir / FString::Printf(TEXT("pt_%02d_%s.png"), Pt.Shots++, *Label);
	if (Delay <= 0.f)
	{
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		return;
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Path]()
	{
		FScreenshotRequest::RequestScreenshot(Path, true, false);
	}), Delay, false);
}

FVector2D ADoubleItGameMode::PlaythroughChooseTap() const
{
	const FDIGame& G = *Game;
	const int32 Num = G.GetRing().Num();

	if (DIPieces::Of(G.GetCenter()).LiftsFromRing())
	{
		// Lift the smallest number: frees space and often sets up a better placement.
		int32 Pick = 0;
		for (int32 i = 1; i < Num; ++i)
		{
			const FDIPiece& P = G.GetRing()[i];
			if (P.IsNumber() && (!G.GetRing()[Pick].IsNumber() || P.Value < G.GetRing()[Pick].Value))
			{
				Pick = i;
			}
		}
		return AtAngle(DIRingLayout::TileAngle(Pick, FMath::Max(Num, 1), RingOffset));
	}

	int32 Slot = 0;
	const int64 AsIs = BestPlacement(G, Slot);
	if (G.CanConvertCenter() && G.GetCenter().IsNumber())
	{
		FDIGame AsPlus = G;
		AsPlus.ConvertCenterToPlus();
		int32 PlusSlot = 0;
		if (BestPlacement(AsPlus, PlusSlot) > AsIs)
		{
			return FVector2D::ZeroVector; // centre tap converts
		}
	}
	return SlotPoint(Slot, Num, RingOffset);
}

bool ADoubleItGameMode::PlaythroughTap(const FVector2D& BoardPos)
{
	ADIPlayerController* PC = Cast<ADIPlayerController>(GetWorld()->GetFirstPlayerController());
	FVector2D Screen;
	if (!PC || !PC->ProjectWorldLocationToScreen(ADIRingActor::ToWorld(BoardPos, 0.f), Screen))
	{
		return false;
	}

	if (Game->IsGameOver())
	{
		PC->SimulateScreenTap(Screen); // restart tap: nothing to predict
		return true;
	}

	// Predict the outcome from the intended board point, then send the tap through the real input path.
	const FDIGame Before = *Game;
	FDIGame Predicted = *Game;
	const bool bPredictedChange = DIRingLayout::ApplyTap(Predicted, BoardPos, RingOffset);

	PC->SimulateScreenTap(Screen);

	const bool bChanged = !SameBoard(*Game, Before) || Game->CanConvertCenter() != Before.CanConvertCenter();
	if (!bPredictedChange)
	{
		UE_LOG(LogDoubleIt, Warning, TEXT("Playthrough: bot chose a tap that does nothing"));
	}
	if (!SameBoard(*Game, Predicted))
	{
		Pt.Mismatches++;
		UE_LOG(LogDoubleIt, Warning, TEXT("Playthrough: tap at (%.0f,%.0f) screen (%.0f,%.0f) gave a different board than predicted"),
			BoardPos.X, BoardPos.Y, Screen.X, Screen.Y);
	}
	return bChanged;
}

void ADoubleItGameMode::PlaythroughStep()
{
#if WITH_EDITOR
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		PlaythroughSchedule(0.5f);
		return;
	}
#endif
	if (IsPlayingMove() || (RingActor && RingActor->IsAnimating()))
	{
		PlaythroughSchedule(0.02f);
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (Pt.bAwaitingSettle)
	{
		const double Settle = Now - Pt.MoveStart;
		Pt.bAwaitingSettle = false;
		Pt.Settles++;
		Pt.SettleSum += Settle;
		Pt.SettleMax = FMath::Max(Pt.SettleMax, Settle);
		if (Pt.Moves == 1 || Pt.Moves % 10 == 0)
		{
			PlaythroughShot(FString::Printf(TEXT("move%02d"), Pt.Moves));
		}
		PlaythroughSchedule(0.35f); // a human-ish pause between moves
		return;
	}

	if (Game->IsGameOver())
	{
		if (!Pt.bSawGameOver)
		{
			Pt.bSawGameOver = true;
			Pt.FinalScore = Game->GetScore();
			PlaythroughShot(TEXT("gameover"), 0.6f);
			PlaythroughSchedule(2.0f);
			return;
		}
		PlaythroughTap(FVector2D(0, -DIRingLayout::RingRadius)); // any tap restarts
		Pt.bRestarted = !Game->IsGameOver() && Game->GetScore() == 0 && Game->GetMoves() == 0;
		PlaythroughShot(TEXT("restart"), 0.8f);
		GetWorldTimerManager().SetTimer(PlaythroughTimer, this, &ADoubleItGameMode::PlaythroughFinish, 2.0f, false);
		return;
	}

	if (Pt.Moves > 400)
	{
		PlaythroughFinish();
		return;
	}

	const EDIPieceKind Kind = Game->GetCenter().Kind;
	const FVector2D Tap = PlaythroughChooseTap();
	const bool bConvert = Tap.IsZero();
	const int32 RingBefore = Game->GetRing().Num();
	const int32 MovesBefore = Game->GetMoves();

	if (!PlaythroughTap(Tap))
	{
		Pt.Rejected++;
		UE_LOG(LogDoubleIt, Warning, TEXT("Playthrough: tap rejected (centre kind %d)"), int32(Kind));
	}

	Pt.Moves++;
	Pt.Minus += Kind == EDIPieceKind::Minus ? 1 : 0;
	Pt.Converts += bConvert ? 1 : 0;
	if (Game->GetMoves() > MovesBefore && Game->GetLastEvents().Num() > 0 && Game->GetRing().Num() < RingBefore)
	{
		const int32 Chain = Game->GetLastEvents().Last().ChainDepth;
		Pt.Fusions++;
		if (Chain > Pt.MaxChain && Pt.Shots < 12)
		{
			PlaythroughShot(FString::Printf(TEXT("chain%d_mid"), Chain), 0.24f + 0.22f * (Chain - 1)); // mid-collision of the last step
		}
		Pt.MaxChain = FMath::Max(Pt.MaxChain, Chain);
	}
	Pt.MoveStart = FPlatformTime::Seconds();
	Pt.bAwaitingSettle = true;
	PlaythroughSchedule(0.02f);
}

void ADoubleItGameMode::PlaythroughFinish()
{
	const double AvgFrameMs = Pt.Frames > 0 ? 1000.0 * Pt.FrameSum / Pt.Frames : 0.0;
	const bool bPass = Pt.Rejected == 0 && Pt.Mismatches == 0 && Pt.bSawGameOver && Pt.bRestarted && Pt.Moves >= 10;
	UE_LOG(LogDoubleIt, Display,
		TEXT("Playthrough: RESULT=%s moves=%d score=%lld fusions=%d maxChain=%d minusUsed=%d converts=%d rejected=%d mismatches=%d ")
		TEXT("restarted=%d settleAvgMs=%.0f settleMaxMs=%.0f avgFrameMs=%.1f worstFrameMs=%.1f"),
		bPass ? TEXT("PASS") : TEXT("FAIL"), Pt.Moves, Pt.FinalScore, Pt.Fusions, Pt.MaxChain, Pt.Minus, Pt.Converts, Pt.Rejected,
		Pt.Mismatches, Pt.bRestarted ? 1 : 0, Pt.Settles ? 1000.0 * Pt.SettleSum / Pt.Settles : 0.0, 1000.0 * Pt.SettleMax,
		AvgFrameMs, 1000.0 * Pt.WorstFrame);
	FPlatformMisc::RequestExit(false, TEXT("DIPlaythrough"));
}
#endif
