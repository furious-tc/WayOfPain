// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SubclassOf.h"
#include "NLGameplaySubsystem.generated.h"

UCLASS()
class UNLGameplaySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    UNLGameplaySubsystem();

    UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = FragmentClass))
    const UNLGameplayItemFragment* FindItemDefinitionFragment(TSubclassOf<UNLGameplayItemDefinition> ItemDef, TSubclassOf<UNLGameplayItemFragment> FragmentClass);
};
