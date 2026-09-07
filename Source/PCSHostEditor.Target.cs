using UnrealBuildTool;
using System.Collections.Generic;

public class PCSHostEditorTarget : TargetRules
{
	public PCSHostEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("PCSHost");
	}
}
