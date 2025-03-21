// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EnigmaVoxel : ModuleRules
{
	public EnigmaVoxel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem","EditorSubsystem", "AIModule", "Niagara", "EnhancedInput","GeometryFramework","MeshDescription", "GeometryCore"});
    }
}
