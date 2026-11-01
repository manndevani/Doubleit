#include "Misc/AutomationTest.h"
#include "Core/DIGame.h"
#include "Core/DIPieceType.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** A fixed, simple player: same calls for the same board, so two identical games stay identical. */
	void ResumePlayOne(FDIGame& Game, int32 Turn)
	{
		if (Game.IsGameOver())
		{
			return;
		}
		const int32 Size = Game.GetRing().Num();
		if (DIPieces::Of(Game.GetCenter()).LiftsFromRing())
		{
			if (Size > 0)
			{
				Game.AbsorbAt((Turn * 5) % Size);
				if (Turn % 2 == 0 && Game.CanConvertCenter())
				{
					Game.ConvertCenterToPlus();
				}
			}
			return;
		}
		Game.PlaceAt((Turn * 7) % (Size + 1));
	}

	bool ResumeSameBoard(const FDIGame& A, const FDIGame& B)
	{
		if (A.GetRing().Num() != B.GetRing().Num() || A.GetScore() != B.GetScore() || A.GetMoves() != B.GetMoves()
			|| A.IsGameOver() != B.IsGameOver() || !(A.GetCenter() == B.GetCenter()) || A.GetCenter().Id != B.GetCenter().Id)
		{
			return false;
		}
		for (int32 i = 0; i < A.GetRing().Num(); ++i)
		{
			if (!(A.GetRing()[i] == B.GetRing()[i]) || A.GetRing()[i].Id != B.GetRing()[i].Id)
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDIRunResumeTest, "DoubleIt.Core.RunResumes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDIRunResumeTest::RunTest(const FString&)
{
	FDIMode Mode = DIModes::Get(DIModes::Classic);
	Mode.Config.Spawn.Find(EDIPieceKind::Plus)->Chance = 0.21f; // a tuned run: the tuning must travel with the save
	int32 Resumed = 0;
	for (int32 Seed = 1; Seed <= 20; ++Seed)
	{
		FDIGame Original(Mode, Seed);
		const int32 SaveAt = 4 + Seed % 9; // early: this simple player loses fast
		for (int32 Turn = 0; Turn < SaveAt; ++Turn)
		{
			ResumePlayOne(Original, Turn);
		}
		if (Original.IsGameOver())
		{
			continue;
		}

		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes);
		TestTrue(TEXT("writes"), Original.SerializeRun(Writer));

		// Loaded into a game built from the plain mode and another seed: everything must come from the save.
		FDIGame Copy(DIModes::Get(DIModes::Classic), 999);
		FMemoryReader Reader(Bytes);
		if (!TestTrue(TEXT("reads"), Copy.SerializeRun(Reader)))
		{
			return false;
		}
		TestTrue(TEXT("same board after load"), ResumeSameBoard(Original, Copy));
		for (int32 Turn = SaveAt; Turn < SaveAt + 200 && !Original.IsGameOver(); ++Turn)
		{
			ResumePlayOne(Original, Turn);
			ResumePlayOne(Copy, Turn);
			if (!ResumeSameBoard(Original, Copy))
			{
				AddError(FString::Printf(TEXT("seed %d diverged at turn %d"), Seed, Turn));
				return false;
			}
		}
		++Resumed;
	}
	AddInfo(FString::Printf(TEXT("STAT run resume: %d of 20 runs saved mid-game and replayed identically"), Resumed));
	TestTrue(TEXT("most runs were still going when saved"), Resumed >= 10);

	// Garbage or a future format is refused, never half-loaded.
	TArray<uint8> Junk = { 9, 9, 9, 9, 1, 2, 3 };
	FMemoryReader JunkReader(Junk);
	FDIGame Fresh(Mode, 1);
	TestFalse(TEXT("unknown format refused"), Fresh.SerializeRun(JunkReader));
	return true;
}

#endif
