// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLGameplayItemFragment_SetStats.h"

#include "Gameplay/NLGameplayItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameplayItemFragment_SetStats)

void UNLGameplayItemFragment_SetStats::OnInstanceCreated(UNLGameplayItemInstance* Instance) const
{
	for (const auto& KVP : InitialItemStats)
	{
		Instance->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

int32 UNLGameplayItemFragment_SetStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}
