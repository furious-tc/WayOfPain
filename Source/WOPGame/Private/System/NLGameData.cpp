// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "System/NLGameData.h"
#include "System/NLAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameData)

UNLGameData::UNLGameData()
{
}

const UNLGameData& UNLGameData::UNLGameData::Get()
{
	return UNLAssetManager::Get().GetGameData();
}
