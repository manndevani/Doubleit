#include "DIGenerateAssetsCommandlet.h"
#include "DoubleIt.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* MaterialsPath = TEXT("/Game/DoubleIt/Materials");

	template <class T>
	T* AddExpression(UMaterial* Material)
	{
		T* Expression = NewObject<T>(Material);
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterialExpressionCustom* AddCustom(UMaterial* Material, const TCHAR* ShaderFile, ECustomMaterialOutputType OutputType,
		const TArray<TPair<FName, UMaterialExpression*>>& Inputs)
	{
		FString Code;
		if (!FFileHelper::LoadFileToString(Code, *(FPaths::ProjectDir() / TEXT("Shaders") / ShaderFile)))
		{
			UE_LOG(LogDoubleIt, Error, TEXT("Missing shader file %s"), ShaderFile);
		}

		UMaterialExpressionCustom* Custom = AddExpression<UMaterialExpressionCustom>(Material);
		Custom->Code = Code;
		Custom->Description = ShaderFile;
		Custom->OutputType = OutputType;
		Custom->Inputs.Reset();
		for (const TPair<FName, UMaterialExpression*>& In : Inputs)
		{
			FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
			Input.InputName = In.Key;
			Input.Input.Connect(0, In.Value);
		}
		return Custom;
	}

	UMaterial* NewMaterial(const TCHAR* Name)
	{
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), MaterialsPath, Name));
		Package->FullyLoad();
		UMaterial* Material = NewObject<UMaterial>(Package, Name, RF_Public | RF_Standalone);
		Material->SetShadingModel(MSM_Unlit);
		return Material;
	}

	bool SaveMaterial(UMaterial* Material)
	{
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		FAssetRegistryModule::AssetCreated(Material);

		UPackage* Package = Material->GetOutermost();
		Package->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Material, *File, Args);
		UE_LOG(LogDoubleIt, Display, TEXT("%s %s"), bSaved ? TEXT("Saved") : TEXT("FAILED to save"), *File);
		return bSaved;
	}

	bool BuildTileMaterial()
	{
		UMaterial* M = NewMaterial(TEXT("M_Tile"));
		M->BlendMode = BLEND_Translucent;
		bool bNeedsRecompile = false;
		M->SetMaterialUsage(bNeedsRecompile, MATUSAGE_InstancedStaticMeshes);

		UMaterialExpression* UV = AddExpression<UMaterialExpressionTextureCoordinate>(M);
		TArray<TPair<FName, UMaterialExpression*>> Inputs = { { TEXT("UV"), UV } };
		const TCHAR* DataNames[] = { TEXT("Value"), TEXT("Kind"), TEXT("Rank"), TEXT("Alpha"), TEXT("Seed") };
		for (uint32 i = 0; i < UE_ARRAY_COUNT(DataNames); ++i)
		{
			UMaterialExpressionPerInstanceCustomData* Data = AddExpression<UMaterialExpressionPerInstanceCustomData>(M);
			Data->DataIndex = i;
			Inputs.Emplace(DataNames[i], Data);
		}

		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(M);
		Inputs.Emplace(TEXT("Time"), Time);

		UMaterialExpressionCustom* Custom = AddCustom(M, TEXT("DI_Tile.hlsl"), CMOT_Float4, Inputs);
		UMaterialEditorOnlyData* Ed = M->GetEditorOnlyData();
		Ed->EmissiveColor.Connect(0, Custom);
		Ed->EmissiveColor.SetMask(1, 1, 1, 1, 0);
		Ed->Opacity.Connect(0, Custom);
		Ed->Opacity.SetMask(1, 0, 0, 0, 1);

		// Idle breathing runs in the vertex shader, so the CPU does nothing while the board is still.
		UMaterialExpressionCustom* Breath = AddCustom(M, TEXT("DI_TileBreath.hlsl"), CMOT_Float3,
			{ { TEXT("Kind"), Inputs[2].Value }, { TEXT("Seed"), Inputs[5].Value }, { TEXT("Time"), Time } });
		Ed->WorldPositionOffset.Connect(0, Breath);
		return SaveMaterial(M);
	}

	bool BuildBackgroundMaterial()
	{
		UMaterial* M = NewMaterial(TEXT("M_Background"));
		M->BlendMode = BLEND_Opaque;

		UMaterialExpressionScreenPosition* Screen = AddExpression<UMaterialExpressionScreenPosition>(M);
		UMaterialExpressionWorldPosition* World = AddExpression<UMaterialExpressionWorldPosition>(M);
		World->WorldPositionShaderOffset = WPT_Default; // absolute: the camera may move to centre the board between HUD bands
		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(M);
		UMaterialExpressionScalarParameter* Fill = AddExpression<UMaterialExpressionScalarParameter>(M);
		Fill->ParameterName = TEXT("Fill");
		Fill->bUseCustomPrimitiveData = true;
		Fill->PrimitiveDataIndex = 0;
		UMaterialExpressionScalarParameter* Dark = AddExpression<UMaterialExpressionScalarParameter>(M);
		Dark->ParameterName = TEXT("Dark");
		Dark->bUseCustomPrimitiveData = true;
		Dark->PrimitiveDataIndex = 1;

		UMaterialExpressionCustom* Custom = AddCustom(M, TEXT("DI_Background.hlsl"), CMOT_Float3,
			{ { TEXT("ScreenUV"), Screen }, { TEXT("WP"), World }, { TEXT("Time"), Time }, { TEXT("Fill"), Fill }, { TEXT("Dark"), Dark } });
		M->GetEditorOnlyData()->EmissiveColor.Connect(0, Custom);
		return SaveMaterial(M);
	}
}
#endif

int32 UDIGenerateAssetsCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const bool bOk = BuildTileMaterial() && BuildBackgroundMaterial();
	return bOk ? 0 : 1;
#else
	return 1;
#endif
}
