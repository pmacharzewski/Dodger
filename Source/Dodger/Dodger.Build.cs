// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Dodger : ModuleRules
{
	public Dodger(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "AIModule", "NavigationSystem" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
