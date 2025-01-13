// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

WOPGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogNL, Log, All);
WOPGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogNLExperience, Log, All);
WOPGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogNLAbilitySystem, Log, All);
WOPGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogNLTeams, Log, All);

WOPGAME_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
