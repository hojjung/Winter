// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Winter2EditorTarget : TargetRules
{
	public Winter2EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("Winter2");
		ExtraModuleNames.AddRange(new string[] {"Winter2Editor"});
	}
}
