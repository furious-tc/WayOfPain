// Copyright 2025 Noblon GmbH. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class WOPGameEditorTarget : TargetRules
{
	public WOPGameEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("WOPGame");
	}
}
