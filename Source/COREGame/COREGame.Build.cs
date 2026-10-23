// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class COREGame : ModuleRules
{
	public COREGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "GameplayAbilities", "GameplayTasks", "GameplayTags", "UMG", "Slate",
			"SlateCore", "AIModule", "Niagara", "OnlineSubsystem", "OnlineSubsystemEOS", "OnlineSubsystemUtils", "Networking", "HTTP", "Json", "MotionWarping", "AnimGraphRuntime", "DeveloperSettings", "NetCore"});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
