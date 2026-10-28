using UnrealBuildTool;

public class OpenCity : ModuleRules
{
    public OpenCity(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.AddRange(new[] { "OpenCity/Public" });
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "AIModule", "NavigationSystem", "GameplayTasks", "UMG",
            "Slate", "SlateCore", "PhysicsCore"
        });
    }
}
