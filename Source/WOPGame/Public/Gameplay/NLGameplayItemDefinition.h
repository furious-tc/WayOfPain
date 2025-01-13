// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "NLGameplayItemDefinition.generated.h"

class UNLGameplayItemInstance;

// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class WOPGAME_API UNLGameplayItemFragment : public UObject {
    GENERATED_BODY()

public:
    virtual void OnInstanceCreated(UNLGameplayItemInstance* Instance) const { }
};

/**
 * UNLGameplayItemDefinition
 *
 *	Non-mutable data asset that contains fragments used to define gameplay item definition.
 */
UCLASS(Blueprintable, Const, Abstract, Meta = (DisplayName = "Gameplay Item Definition", ShortTooltip = "Data asset used to define gameplay item definitions."))
class UNLGameplayItemDefinition: public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UNLGameplayItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayItem)
    FName GameplayItemName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayItem, Instanced)
    TArray<TObjectPtr<UNLGameplayItemFragment>> Fragments;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure = false, meta = (DeterminesOutputType = FragmentClass))
    const UNLGameplayItemFragment* FindFragmentByClass(TSubclassOf<UNLGameplayItemFragment> FragmentClass) const;

};
