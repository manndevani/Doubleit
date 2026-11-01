#include "DoubleIt.h"
#include "Internationalization/StringTableRegistry.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDoubleIt);

class FDoubleItModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Literal path (relative to Content/) so the localization gatherer finds the table. See Game/DIText.h.
		LOCTABLE_FROMFILE_GAME("DoubleIt", "DoubleIt", "DoubleIt/Text/DoubleIt.csv");
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FDoubleItModule, DoubleIt, "DoubleIt");
