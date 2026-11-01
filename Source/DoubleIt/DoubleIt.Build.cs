using System.IO;
using UnrealBuildTool;

public class DoubleIt : ModuleRules
{
	public DoubleIt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "AssetRegistry", "UMG", "Slate", "SlateCore", "DeveloperSettings" });

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "DoubleIt_UPL.xml"));
		}
	}
}
