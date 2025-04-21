// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EnigmaVoxelTarget : TargetRules
{
	public EnigmaVoxelTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("EnigmaVoxel");
	}
}