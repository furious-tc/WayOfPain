// Copyright 2025 Noblon GmbH. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using EpicGames.Core;
using System.Collections.Generic;
using UnrealBuildBase;
using Microsoft.Extensions.Logging;

public class WOPGameTarget : TargetRules
{
	public WOPGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.Add("WOPGame");

        WOPGameTarget.ApplySharedNLTargetSettings(this);
    }

    private static bool bHasWarnedAboutShared = false;
    internal static void ApplySharedNLTargetSettings(TargetRules Target)
    {
        ILogger Logger = Target.Logger;

        Target.DefaultBuildSettings = BuildSettingsVersion.V5;
        Target.IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        bool bIsTest = Target.Configuration == UnrealTargetConfiguration.Test;
        bool bIsShipping = Target.Configuration == UnrealTargetConfiguration.Shipping;
        bool bIsDedicatedServer = Target.Type == TargetType.Server;
        if (Target.BuildEnvironment == TargetBuildEnvironment.Unique)
        {
            Target.ShadowVariableWarningLevel = WarningLevel.Error;

            Target.bUseLoggingInShipping = true;

            if (bIsShipping && !bIsDedicatedServer)
            {
                // Make sure that we validate certificates for HTTPS traffic
                Target.bDisableUnverifiedCertificates = true;

                // Uncomment these lines to lock down the command line processing
                // This will only allow the specified command line arguments to be parsed
                //Target.GlobalDefinitions.Add("UE_COMMAND_LINE_USES_ALLOW_LIST=1");
                //Target.GlobalDefinitions.Add("UE_OVERRIDE_COMMAND_LINE_ALLOW_LIST=\"-space -separated -list -of -commands\"");

                // Uncomment this line to filter out sensitive command line arguments that you
                // don't want to go into the log file (e.g., if you were uploading logs)
                //Target.GlobalDefinitions.Add("FILTER_COMMANDLINE_LOGGING=\"-some_connection_id -some_other_arg\"");
            }

            if (bIsShipping || bIsTest)
            {
                // Disable reading generated/non-ufs ini files
                Target.bAllowGeneratedIniWhenCooked = false;
                Target.bAllowNonUFSIniWhenCooked = false;
            }

            if (Target.Type != TargetType.Editor)
            {
                // We don't use the path tracer at runtime, only for beauty shots, and this DLL is quite large
                Target.DisablePlugins.Add("OpenImageDenoise");

                // Reduce memory use in AssetRegistry always-loaded data, but add more cputime expensive queries
                Target.GlobalDefinitions.Add("UE_ASSETREGISTRY_INDIRECT_ASSETDATA_POINTERS=1");
            }
        }
        else
        {
            // !!!!!!!!!!!! WARNING !!!!!!!!!!!!!
            // Any changes in here must not affect PCH generation, or the target
            // needs to be set to TargetBuildEnvironment.Unique

            // This only works in editor or Unique build environments
            if (Target.Type != TargetType.Editor)
            {
                // Shared monolithic builds cannot enable/disable plugins or change any options because it tries to re-use the installed engine binaries
                if (!bHasWarnedAboutShared)
                {
                    bHasWarnedAboutShared = true;
                    Logger.LogWarning("Dynamic target options are disabled when packaging from an installed version of the engine");
                }
            }
        }
    }

    static public bool ShouldEnableAllGameFeaturePlugins(TargetRules Target)
    {
        if (Target.Type == TargetType.Editor)
        {
            // With return true, editor builds will build all game feature plugins, but it may or may not load them all.
            // This is so you can enable plugins in the editor without needing to compile code.
            // return true;
        }

        bool bIsBuildMachine = (Environment.GetEnvironmentVariable("IsBuildMachine") == "1");
        if (bIsBuildMachine)
        {
            // This could be used to enable all plugins for build machines
            // return true;
        }

        // By default use the default plugin rules as set by the plugin browser in the editor
        // This is important because this code may not be run at all for launcher-installed versions of the engine
        return false;
    }
}
