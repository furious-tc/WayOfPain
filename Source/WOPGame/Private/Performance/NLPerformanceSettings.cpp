// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Performance/NLPerformanceSettings.h"

#include "Engine/PlatformSettingsManager.h"
#include "Misc/EnumRange.h"
#include "Performance/NLPerformanceStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPerformanceSettings)

//////////////////////////////////////////////////////////////////////

UNLPlatformSpecificRenderingSettings::UNLPlatformSpecificRenderingSettings()
{
	MobileFrameRateLimits.Append({ 20, 30, 45, 60, 90, 120 });
}

const UNLPlatformSpecificRenderingSettings* UNLPlatformSpecificRenderingSettings::Get()
{
	UNLPlatformSpecificRenderingSettings* Result = UPlatformSettingsManager::Get().GetSettingsForPlatform<ThisClass>();
	check(Result);
	return Result;
}

//////////////////////////////////////////////////////////////////////

UNLPerformanceSettings::UNLPerformanceSettings()
{
	PerPlatformSettings.Initialize(UNLPlatformSpecificRenderingSettings::StaticClass());

	CategoryName = TEXT("Game");

	DesktopFrameRateLimits.Append({ 30, 60, 120, 144, 160, 165, 180, 200, 240, 360 });

	// Default to all stats are allowed
	FNLPerformanceStatGroup& StatGroup = UserFacingPerformanceStats.AddDefaulted_GetRef();
	for (ENLDisplayablePerformanceStat PerfStat : TEnumRange<ENLDisplayablePerformanceStat>())
	{
		StatGroup.AllowedStats.Add(PerfStat);
	}
}

