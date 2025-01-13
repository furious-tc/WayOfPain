// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLGameplayItemDefinition.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"

UNLGameplayItemDefinition::UNLGameplayItemDefinition(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

const UNLGameplayItemFragment* UNLGameplayItemDefinition::FindFragmentByClass(TSubclassOf<UNLGameplayItemFragment> FragmentClass) const
{
    if (FragmentClass != nullptr) {
        for (UNLGameplayItemFragment* Fragment : Fragments) {
            if (Fragment && Fragment->IsA(FragmentClass)) {
                return Fragment;
            }
        }
    }

    return nullptr;
}
