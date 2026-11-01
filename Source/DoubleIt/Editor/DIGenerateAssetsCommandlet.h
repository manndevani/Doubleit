#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DIGenerateAssetsCommandlet.generated.h"

/**
 * Regenerates the project's materials from Shaders/(name).hlsl so nothing is hand-wired in the editor.
 * Run: UnrealEditor-Cmd Doubleit.uproject -run=DIGenerateAssets   (or Scripts/generate_assets.ps1)
 */
UCLASS()
class UDIGenerateAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
