using UnrealBuildTool;
using System.Collections.Generic;

public class OpenCityTarget : TargetRules
{
    public OpenCityTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("OpenCity");
    }
}
