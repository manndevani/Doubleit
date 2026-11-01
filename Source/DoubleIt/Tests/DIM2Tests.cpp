#include "Misc/AutomationTest.h"
#include "Core/DIAdaptive.h"
#include "Core/DIGame.h"
#include "Core/DIPieceType.h"
#include "Core/DIRingLayout.h"
#include "Core/DISea.h"
#include "Core/DISynth.h"
#include "Core/DITutorial.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags DIM2TestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIFusionEventsTest, "DoubleIt.Core.FusionEventsTrackIds", DIM2TestFlags)
bool FDIFusionEventsTest::RunTest(const FString&)
{
	FDIGame Game(DIModes::Get(DIModes::Classic), 1);
	Game.SetBoard({ FDIPiece::Number(8), FDIPiece::Number(2), FDIPiece::Number(2), FDIPiece::Number(8) }, FDIPiece::Plus());
	const TArray<FDIPiece> Before = Game.GetRing();
	const uint32 PlusId = Game.GetCenter().Id;
	Game.PlaceAt(2);

	const TArray<FDIFusionEvent>& Events = Game.GetLastEvents();
	if (!TestEqual(TEXT("two fusions"), Events.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("first consumes left 2"), Events[0].ConsumedIds[0], Before[1].Id);
	TestEqual(TEXT("first consumes right 2"), Events[0].ConsumedIds[1], Before[2].Id);
	TestEqual(TEXT("first consumes the plus"), Events[0].ConsumedIds[2], PlusId);
	TestEqual(TEXT("chain consumes first result"), Events[1].ConsumedIds[2], Events[0].ResultId);
	TestEqual(TEXT("final ring holds chain result"), Game.GetRing()[0].Id, Events[1].ResultId);

	TSet<uint32> Ids;
	for (const FDIPiece& P : Game.GetRing())
	{
		Ids.Add(P.Id);
	}
	TestFalse(TEXT("center id unique"), Ids.Contains(Game.GetCenter().Id));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDISynthTest, "DoubleIt.Core.Synth", DIM2TestFlags)
bool FDISynthTest::RunTest(const FString&)
{
	TestTrue(TEXT("step 0 is C5"), FMath::IsNearlyEqual(DISynth::PentatonicFrequency(0), 523.25f, 0.1f));
	TestTrue(TEXT("step 5 is C6"), FMath::IsNearlyEqual(DISynth::PentatonicFrequency(5), 1046.5f, 0.2f));
	TestTrue(TEXT("step -5 is C4"), FMath::IsNearlyEqual(DISynth::PentatonicFrequency(-5), 261.63f, 0.1f));
	TestTrue(TEXT("step -1 is A4"), FMath::IsNearlyEqual(DISynth::PentatonicFrequency(-1), 440.f, 0.1f));

	// Chords: step 0 is the chord's first tone, one octave per Tones.Num() steps.
	const TArray<int32> G6add9 = { 2, 4, 7, 9, 11 };
	TestTrue(TEXT("chord step 0 is D5"), FMath::IsNearlyEqual(DISynth::ScaleFrequency(0, G6add9, 523.2511f), 587.33f, 0.1f));
	TestTrue(TEXT("chord step -5 is D4"), FMath::IsNearlyEqual(DISynth::ScaleFrequency(-5, G6add9, 523.2511f), 293.66f, 0.1f));

	const DISynth::FVoice Voice;
	const TArray<int16> Pcm = DISynth::Note(Voice, 440.f, 0.5f);
	TestEqual(TEXT("stereo length"), Pcm.Num(), 2 * FMath::RoundToInt32(Voice.Seconds * DISynth::SampleRate));
	int32 Peak = 0;
	double Head = 0, Tail = 0, Side = 0;
	for (int32 i = 0; i < Pcm.Num(); ++i)
	{
		const int32 Abs = FMath::Abs(int32(Pcm[i]));
		Peak = FMath::Max(Peak, Abs);
		if (i < Pcm.Num() / 10)
		{
			Head += Abs;
		}
		else if (i >= Pcm.Num() * 9 / 10)
		{
			Tail += Abs;
		}
		if (i % 2 == 0)
		{
			Side += FMath::Abs(int32(Pcm[i]) - int32(Pcm[i + 1]));
		}
	}
	TestTrue(TEXT("never clips at half volume"), Peak <= 16384 + 64);
	TestTrue(TEXT("audible"), Peak > 4000);
	TestTrue(TEXT("decays"), Tail < Head * 0.1);
	TestTrue(TEXT("wide: left and right differ"), Side > 0);
	TestEqual(TEXT("starts silent (no click)"), int32(Pcm[0]), 0);
	TestTrue(TEXT("ends silent (no click)"), FMath::Abs(int32(Pcm.Last())) < 64);

	// Pad loop: right length, and the seam (last frame -> first frame) is no bigger than a normal step.
	const DISynth::FAmbience Ambience;
	{
		const TArray<int16> Loop = DISynth::Pad(Ambience, Ambience.PadHz, Ambience.PadLevel);
		TestEqual(TEXT("loop length"), Loop.Num(), 2 * Ambience.LoopSeconds * DISynth::AmbienceRate);
		int32 MaxStep = 0;
		for (int32 i = 2; i < Loop.Num(); ++i)
		{
			MaxStep = FMath::Max(MaxStep, FMath::Abs(int32(Loop[i]) - int32(Loop[i - 2])));
		}
		const int32 Seam = FMath::Max(FMath::Abs(int32(Loop[0]) - int32(Loop[Loop.Num() - 2])), FMath::Abs(int32(Loop[1]) - int32(Loop.Last())));
		TestTrue(TEXT("seamless loop"), Seam <= MaxStep);
	}

	// Live sea: rendering in blocks continues seamlessly, it never clips, and its level moves (waves), never a flat hiss.
	constexpr int32 Rate = DISynth::AmbienceRate;
	FDISea Whole(Rate, 11), Blocks(Rate, 11);
	TArray<int16> A, B;
	Whole.Render(A, Rate * 60);
	for (int32 i = 0; i < 60 * 5; ++i)
	{
		Blocks.Render(B, Rate / 5); // 22050 / 5 is whole: 300 blocks are exactly 60 s
	}
	if (!TestEqual(TEXT("sea length"), B.Num(), A.Num()))
	{
		return false;
	}
	int32 MaxDiff = 0, SeaPeak = 0;
	for (int32 i = 0; i < A.Num(); ++i)
	{
		MaxDiff = FMath::Max(MaxDiff, FMath::Abs(int32(A[i]) - int32(B[i])));
		SeaPeak = FMath::Max(SeaPeak, FMath::Abs(int32(A[i])));
	}
	TestTrue(TEXT("blocks continue seamlessly"), MaxDiff <= 2);
	TestTrue(TEXT("sea never clips"), SeaPeak < 32767);
	TArray<double> Level; // RMS per half second
	for (int32 w = 0; w < 120; ++w)
	{
		double Sum = 0;
		for (int32 i = 0; i < Rate; ++i) // Rate samples = half a second of stereo
		{
			Sum += FMath::Square(double(A[w * Rate + i]));
		}
		Level.Add(FMath::Sqrt(Sum / Rate));
	}
	double Mean = 0, Var = 0;
	for (double L : Level) Mean += L / Level.Num();
	for (double L : Level) Var += FMath::Square(L - Mean) / Level.Num();
	AddInfo(FString::Printf(TEXT("STAT sea rms mean %.0f, swing %.0f%%, peak %d"), Mean, 100.0 * FMath::Sqrt(Var) / Mean, SeaPeak));
	TestTrue(TEXT("audible"), Mean > 1000.0);
	TestTrue(TEXT("waves rise and fall"), FMath::Sqrt(Var) > 0.2 * Mean);
	FDISea Other(Rate, 12);
	TArray<int16> C;
	Other.Render(C, Rate * 2);
	TestTrue(TEXT("another seed is another sea"), FMemory::Memcmp(C.GetData(), A.GetData(), C.Num() * sizeof(int16)) != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDITutorialTest, "DoubleIt.Core.TutorialHappyPathAndRetry", DIM2TestFlags)
bool FDITutorialTest::RunTest(const FString&)
{
	auto At = [](float Degrees)
	{
		const float Rad = FMath::DegreesToRadians(Degrees);
		return FVector2D(DIRingLayout::RingRadius * FMath::Sin(Rad), DIRingLayout::RingRadius * FMath::Cos(Rad));
	};

	FDIGame Game(DIModes::Get(DIModes::Classic), 3);
	FDITutorial Tut;
	Tut.SetupStep(Game);

	// 0: any placement
	DIRingLayout::ApplyTap(Game, At(10));
	TestTrue(TEXT("step 0 advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);

	// 1: [2,2,8,4] plus; wrong gap first, then the right one (between index 0 and 1 = 45 deg)
	Tut.SetupStep(Game);
	{
		FDIGame Trial = Game;
		DIRingLayout::ApplyTap(Trial, At(200));
		TestTrue(TEXT("a wrong gap is judged Retry on a copy"), Tut.Judge(Trial) == EDITutorialResult::Retry);
		TestEqual(TEXT("the real board is untouched"), Game.GetRing().Num(), 4);
	}
	DIRingLayout::ApplyTap(Game, At(200));
	TestTrue(TEXT("wrong gap retries"), Tut.Evaluate(Game) == EDITutorialResult::Retry);
	Tut.SetupStep(Game);
	DIRingLayout::ApplyTap(Game, At(45));
	TestTrue(TEXT("merge advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);

	// 2: [8,2,2,8,1,4] plus between the 2s (index 1..2 = 60..120 deg)
	Tut.SetupStep(Game);
	DIRingLayout::ApplyTap(Game, At(90));
	TestTrue(TEXT("chain advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);

	// 3: minus absorbs any tile
	Tut.SetupStep(Game);
	DIRingLayout::ApplyTap(Game, At(0));
	TestTrue(TEXT("absorb advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);

	// 4: center tap converts
	Tut.SetupStep(Game);
	TestTrue(TEXT("step 4 starts with absorbed centre"), Game.CanConvertCenter());
	{
		// The game mode tries taps on a copy: placing the lifted piece is judged Retry, so it is never played.
		FDIGame Trial = Game;
		DIRingLayout::ApplyTap(Trial, At(30));
		TestTrue(TEXT("placing the lifted piece is refused"), Tut.Judge(Trial) == EDITutorialResult::Retry);
		TestEqual(TEXT("judging does not move the tutorial on"), Tut.GetStep(), 4);
	}
	DIRingLayout::ApplyTap(Game, FVector2D::ZeroVector);
	TestTrue(TEXT("convert advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);

	// 5: breeze, brought in with the HUD button (a ring tap with the number is refused)
	Tut.SetupStep(Game);
	const int32 Before = Game.GetRing().Num();
	{
		FDIGame Trial = Game;
		DIRingLayout::ApplyTap(Trial, At(10));
		TestTrue(TEXT("placing the number instead is refused"), Tut.Judge(Trial) == EDITutorialResult::Retry);
	}
	TestTrue(TEXT("the step teaches the breeze"), Tut.BringInKind() == EDIPieceKind::Breeze);
	Game.BringIn(Tut.BringInKind().GetValue());
	DIRingLayout::ApplyTap(Game, At(10));
	TestEqual(TEXT("breeze halves"), Game.GetRing().Num(), Before - Before / 2);
	TestTrue(TEXT("last step advances"), Tut.Evaluate(Game) == EDITutorialResult::Advance);
	TestTrue(TEXT("finished"), Tut.IsFinished());
	return true;
}

namespace
{
	struct FDISimResult { int64 Moves = 0, Score = 0, Highest = 0; };

	/** Average of Games greedy-bot runs (lifts the smallest number with a minus, else best immediate gain). */
	FDISimResult SimulateGreedy(FDIMode Mode, const FDISpawnTuning& Tuning, int32 Games)
	{
		FDISimResult Total;
		for (int32 Seed = 0; Seed < Games; ++Seed)
		{
			Mode.Config.Spawn = Tuning;
			FDIGame Game(Mode, 1000 + Seed);
			FRandomStream Bot(Seed);
			int32 Guard = 0;
			while (!Game.IsGameOver() && ++Guard < 3000)
			{
				const int32 Num = Game.GetRing().Num();
				if (Game.GetCenter().Kind == EDIPieceKind::Minus)
				{
					// Lift the smallest number: a plausible human habit.
					int32 Pick = 0;
					for (int32 i = 1; i < Num; ++i)
					{
						if (Game.GetRing()[i].Value < Game.GetRing()[Pick].Value) Pick = i;
					}
					Game.AbsorbAt(Pick);
					continue;
				}
				int32 BestSlot = Bot.RandRange(0, Num);
				int64 BestGain = 0;
				for (int32 Slot = 0; Slot <= Num; ++Slot)
				{
					FDIGame Trial = Game;
					Trial.PlaceAt(Slot);
					const int64 Gain = (Trial.GetScore() - Game.GetScore()) * 4 - Trial.GetRing().Num();
					if (Gain > BestGain)
					{
						BestGain = Gain;
						BestSlot = Slot;
					}
				}
				Game.PlaceAt(BestSlot);
			}
			Total.Moves += Game.GetMoves();
			Total.Score += Game.GetScore();
			Total.Highest += Game.GetHighestValue();
		}
		return { Total.Moves / Games, Total.Score / Games, Total.Highest / Games };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIGreedyBalanceTest, "DoubleIt.Sim.GreedyBalance", DIM2TestFlags)
bool FDIGreedyBalanceTest::RunTest(const FString&)
{
	for (const TCHAR* Name : { DIModes::Classic, DIModes::Addition })
	{
		const FDISimResult R = SimulateGreedy(DIModes::Get(Name), FDISpawnTuning(), 20);
		AddInfo(FString::Printf(TEXT("STAT %s greedy bot: avg moves %lld, avg score %lld, avg highest %lld"), Name, R.Moves, R.Score, R.Highest));
		TestTrue(FString::Printf(TEXT("%s runs last a while"), Name), R.Moves >= 40);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIAdaptiveTest, "DoubleIt.Core.AdaptivePace", DIM2TestFlags)
bool FDIAdaptiveTest::RunTest(const FString&)
{
	const FDISpawnTuning Base;
	FDIPlayerProfile Fresh;
	TestEqual(TEXT("no data is neutral"), DIAdaptive::Skill(Fresh), 0.5f, 0.001f);
	TestEqual(TEXT("neutral keeps the plus chance"), DIAdaptive::TuningFor(Fresh).ChanceOf(EDIPieceKind::Plus), Base.ChanceOf(EDIPieceKind::Plus), 0.001f);

	FDIPlayerProfile Struggling;
	FDIPlayerProfile Strong;
	for (int32 i = 0; i < 8; ++i)
	{
		DIAdaptive::Learn(Struggling, { 22, 1, 6, 2, 3.f, 16 });
		DIAdaptive::Learn(Strong, { 160, 5, 20, 19, 2.f, 1024 });
	}
	TestEqual(TEXT("runs counted"), Struggling.Runs, 8);
	TestEqual(TEXT("highest kept"), Strong.HighestEver, int64(1024));
	TestTrue(TEXT("struggling reads low"), DIAdaptive::Skill(Struggling) < 0.25f);
	TestTrue(TEXT("strong reads high"), DIAdaptive::Skill(Strong) > 0.75f);

	const FDISpawnTuning Easy = DIAdaptive::TuningFor(Struggling);
	const FDISpawnTuning Hard = DIAdaptive::TuningFor(Strong);
	TestTrue(TEXT("help: more pluses"), Easy.ChanceOf(EDIPieceKind::Plus) > Base.ChanceOf(EDIPieceKind::Plus) && Easy.ChanceOf(EDIPieceKind::Plus) <= 0.22f);
	TestTrue(TEXT("help: slower growth"), Easy.MovesPerRangeStep > Base.MovesPerRangeStep && Easy.MovesPerRangeStep <= 50);
	TestTrue(TEXT("push: fewer pluses"), Hard.ChanceOf(EDIPieceKind::Plus) < Base.ChanceOf(EDIPieceKind::Plus) && Hard.ChanceOf(EDIPieceKind::Plus) >= 0.14f);
	TestTrue(TEXT("push: faster growth"), Hard.MovesPerRangeStep < Base.MovesPerRangeStep && Hard.MovesPerRangeStep >= 34);

	// One bad run after a strong history only nudges the estimate.
	FDIPlayerProfile OneBad = Strong;
	DIAdaptive::Learn(OneBad, { 20, 1, 5, 1, 2.f, 8 });
	TestTrue(TEXT("one run is a nudge"), DIAdaptive::Skill(OneBad) > 0.6f);

	// The tuning really changes how long a run lasts, in the intended direction.
	const FDISimResult EasyRun = SimulateGreedy(DIModes::Get(DIModes::Classic), Easy, 20);
	const FDISimResult BaseRun = SimulateGreedy(DIModes::Get(DIModes::Classic), Base, 20);
	const FDISimResult HardRun = SimulateGreedy(DIModes::Get(DIModes::Classic), Hard, 20);
	AddInfo(FString::Printf(TEXT("STAT adaptive greedy moves: helped %lld, base %lld, pushed %lld"), EasyRun.Moves, BaseRun.Moves, HardRun.Moves));
	TestTrue(TEXT("helped runs last longer"), EasyRun.Moves > BaseRun.Moves);
	TestTrue(TEXT("pushed runs are not longer"), HardRun.Moves <= BaseRun.Moves);
	return true;
}

#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDITutorialHintTest, "DoubleIt.Core.TutorialHintCompletesEachStep", DIM2TestFlags)
bool FDITutorialHintTest::RunTest(const FString&)
{
	// Following the coral hint alone must walk through the whole tutorial, at any ring rotation.
	for (const float Offset : { 0.f, 0.7f })
	{
		FDIGame Game(DIModes::Get(DIModes::Classic), 5);
		FDITutorial Tut;
		while (!Tut.IsFinished())
		{
			const int32 Step = Tut.GetStep();
			Tut.SetupStep(Game);
			if (Tut.BringInKind())
			{
				TestFalse(TEXT("no ring hint before the breeze is in"), Tut.FindTarget(Game, Offset).IsSet());
				Game.BringIn(Tut.BringInKind().GetValue()); // what the pulsing HUD button does
			}
			const TOptional<FDITapTarget> Target = Tut.FindTarget(Game, Offset);
			if (!TestTrue(FString::Printf(TEXT("step %d has a hint"), Step), Target.IsSet()))
			{
				return false;
			}
			if (Target->Pos != FVector2D::ZeroVector && DIPieces::Of(Game.GetCenter()).LiftsFromRing())
			{
				// A piece that lifts points at the stone itself, never at the gap beside it.
				bool bOnStone = false;
				for (int32 i = 0; i < Game.GetRing().Num(); ++i)
				{
					bOnStone |= Target->Pos.Equals(DIRingLayout::TilePosition(i, Game.GetRing().Num(), Offset), 0.01f);
				}
				TestTrue(FString::Printf(TEXT("step %d hint sits on a stone"), Step), bOnStone);
			}
			DIRingLayout::ApplyTap(Game, Target->Pos, Offset);
			if (!TestTrue(FString::Printf(TEXT("tapping the hint completes step %d"), Step), Tut.Evaluate(Game) == EDITutorialResult::Advance))
			{
				return false;
			}
		}
	}
	return true;
}
