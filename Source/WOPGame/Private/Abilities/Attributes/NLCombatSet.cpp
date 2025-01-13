// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Abilities/Attributes/NLCombatSet.h"
#include "Abilities/Attributes/NLAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLCombatSet)

class FLifetimeProperty;


UNLCombatSet::UNLCombatSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UNLCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UNLCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UNLCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UNLCombatSet, BaseReceivedDamageScale, COND_OwnerOnly, REPNOTIFY_Always);
}

void UNLCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNLCombatSet, BaseDamage, OldValue);
}

void UNLCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNLCombatSet, BaseHeal, OldValue);
}

void UNLCombatSet::OnRep_BaseReceivedDamageScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNLCombatSet, BaseReceivedDamageScale, OldValue);
}
