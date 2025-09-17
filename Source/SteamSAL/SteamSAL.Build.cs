using UnrealBuildTool;

public class SteamSAL : ModuleRules
{
    public SteamSAL(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine"
        });

        // Add these *only if* you actually use them in code elsewhere.
        PrivateDependencyModuleNames.AddRange(new string[] {
            "OnlineSubsystem"
        });

        // This is the key line that exposes the Steamworks SDK and links steam_api.
        AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
    }
}
