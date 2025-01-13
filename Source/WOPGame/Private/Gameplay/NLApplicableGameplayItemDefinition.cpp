// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLApplicableGameplayItemDefinition.h"
#include "Gameplay/NLApplicableGameplayItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLApplicableGameplayItemDefinition)

UNLApplicableGameplayItemDefinition::UNLApplicableGameplayItemDefinition(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstanceType = UNLApplicableGameplayItemInstance::StaticClass();
}
