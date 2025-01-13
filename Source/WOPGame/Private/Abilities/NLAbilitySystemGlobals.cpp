// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Abilities/NLAbilitySystemGlobals.h"
#include "Abilities/NLGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLAbilitySystemGlobals)

struct FGameplayEffectContext;

UNLAbilitySystemGlobals::UNLAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UNLAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FNLGameplayEffectContext();
}

