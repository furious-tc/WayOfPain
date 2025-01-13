// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Abilities/Attributes/NLAttributeSet.h"
#include "Abilities/NLAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLAttributeSet)

class UWorld;

UNLAttributeSet::UNLAttributeSet()
{
}

UWorld* UNLAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UNLAbilitySystemComponent* UNLAttributeSet::GetNLAbilitySystemComponent() const
{
	return Cast<UNLAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

