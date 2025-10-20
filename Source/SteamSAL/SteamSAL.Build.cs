using UnrealBuildTool;

public class SteamSAL : ModuleRules
{
    public SteamSAL(ReadOnlyTargetRules Target) : base(Target)
    {
        // Keep explicit/shared PCHs enabled (Fab may build Engine SharedPCH)
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

        // ------------------------------------------------------------------
        // Fix for UE 5.0–5.4 on MSVC builders (Fab): some Engine headers use
        // __has_feature / __has_extension etc. during SharedPCH compilation.
        // On MSVC those probes may be undefined and (with strict flags)
        // cause C4668/C4067 in SharedPCH.Engine.ShadowErrors.cpp.
        //
        // Defining them to 0 makes '#if __has_feature(...)' evaluate cleanly.
        // We only apply this on Windows and for UE <= 5.4 so newer engines
        // aren’t affected.
        // ------------------------------------------------------------------
        bool IsUE54OrOlder =
    (Target.Version.MajorVersion == 5) && (Target.Version.MinorVersion <= 4);

    if (Target.Platform == UnrealTargetPlatform.Win64 && IsUE54OrOlder)
    {
        PublicDefinitions.Add("__has_feature(x)=0");
        PublicDefinitions.Add("__has_extension(x)=0");
        PublicDefinitions.Add("__is_identifier(x)=0");

    }

    }
}
