// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Gameplay/NLGameplayItemDefinition.h"

#include "NLGameplayItemFragment_SetStats.generated.h"

class UNLGameplayItemInstance;
class UObject;
struct FGameplayTag;

UCLASS()
class UNLGameplayItemFragment_SetStats : public UNLGameplayItemFragment
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;

public:
	virtual void OnInstanceCreated(UNLGameplayItemInstance* Instance) const override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
};
