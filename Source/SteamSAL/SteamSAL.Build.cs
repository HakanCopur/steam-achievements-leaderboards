using UnrealBuildTool;

public class SteamSAL : ModuleRules
{
    public SteamSAL(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new[] {
            "OnlineSubsystem",
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Use Engine’s bundled Steamworks; define a guard for your .cpp files
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
            PrivateDefinitions.Add("WITH_STEAMWORKS=1");
        }
        else
        {
            PrivateDefinitions.Add("WITH_STEAMWORKS=0");
        }
    }
}
