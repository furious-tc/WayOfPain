// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLGameplaySubsystem.h"
#include "Gameplay/NLGameplayItemDefinition.h"

UNLGameplaySubsystem::UNLGameplaySubsystem()
{
}

const UNLGameplayItemFragment* UNLGameplaySubsystem::FindItemDefinitionFragment(TSubclassOf<UNLGameplayItemDefinition> ItemDef, TSubclassOf<UNLGameplayItemFragment> FragmentClass)
{
    if ((ItemDef != nullptr) && (FragmentClass != nullptr)) {
        return GetDefault<UNLGameplayItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
    }

    return nullptr;
}
