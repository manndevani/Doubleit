using UnrealBuildTool;

public class DoubleItTarget : TargetRules
{
	public DoubleItTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DoubleIt");
	}
}
