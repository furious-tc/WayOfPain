// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Gameplay/NLGameplayItemDefinition.h"
#include "Templates/SubclassOf.h"
#include "NLGameplayItemFragment_ApplicableGameplayItemDefinition.generated.h"

class UNLApplicableGameplayItemDefinition;
class UObject;

UCLASS()

class UNLGameplayItemFragment_ApplicableGameplayItemDefinition : public UNLGameplayItemFragment
{
	GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayItem)
	TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition;
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayItem)
    FGameplayTagContainer GameplayItemCategoryTags;

    // Cannot add more than this count on a similar category definition.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayItem)
    int32 StackLimit = 1;

    // If the category is unique, all existing items of any exact matching tag will be removed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayItem)
    bool bUniqueCategory = false;

    // Whether to apply the gameplay item once gathered.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayItem)
    bool bApplyGameplayItemOnGather = false;
};
