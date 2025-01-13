// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "NLAttributeSet.h"
#include "NLCombatSet.generated.h"

class UObject;
struct FFrame;

/**
 * UNLCombatSet
 *
 *  Class that defines attributes that are necessary for applying damage or healing.
 *	Attribute examples include: damage, healing, attack power, and shield penetrations.
 */
UCLASS(BlueprintType)
class UNLCombatSet : public UNLAttributeSet
{
	GENERATED_BODY()

public:

	UNLCombatSet();

	ATTRIBUTE_ACCESSORS(UNLCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UNLCombatSet, BaseHeal);
	ATTRIBUTE_ACCESSORS(UNLCombatSet, BaseReceivedDamageScale);

protected:

	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseReceivedDamageScale(const FGameplayAttributeData& OldValue);

private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "NL|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "NL|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;

	// The base amount of damage scale to add percentage with in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseReceivedDamageScale, Category = "NL|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseReceivedDamageScale;
};
