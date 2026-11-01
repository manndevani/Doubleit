using UnrealBuildTool;

public class DoubleItEditorTarget : TargetRules
{
	public DoubleItEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DoubleIt");
	}
}
