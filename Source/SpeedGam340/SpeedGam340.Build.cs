// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpeedGam340 : ModuleRules
{
	public SpeedGam340(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
