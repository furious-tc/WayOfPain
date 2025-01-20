// Copyright 2025 Noblon GmbH. All Rights Reserved.

using UnrealBuildTool;

public class WOPGame : ModuleRules
{
	public WOPGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
        PublicIncludePaths.AddRange(
			new string[] {
                "WOPGame"
            }
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "ModularGameplay", "NavigationSystem", "Niagara", "CommonLoadingScreen", "ApplicationCore", "AsyncMixin", "PhysicsCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "Slate", "SlateCore", "UMG", "GameplayAbilities", "GameplayTags", "GameplayTasks", "GameplayMessageRuntime", "AudioModulation", "CommonUI", "CommonInput", "AudioMixer", "DeveloperSettings", "RHI", "CommonGame", "UIExtension" });

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
            }
        );

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
