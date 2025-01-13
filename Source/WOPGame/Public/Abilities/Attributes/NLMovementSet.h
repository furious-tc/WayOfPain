// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Abilities/Attributes/NLAttributeSet.h"
#include "NativeGameplayTags.h"

#include "NLMovementSet.generated.h"

class UObject;
struct FFrame;

struct FGameplayEffectModCallbackData;

/**
 * UNLMovementSet
 *
 *	Class that defines attributes that are necessary for managing movement.
 *	Attribute examples include: Movement speed.
 */
UCLASS(BlueprintType)
class UNLMovementSet : public UNLAttributeSet
{
	GENERATED_BODY()

public:

	UNLMovementSet();

	ATTRIBUTE_ACCESSORS(UNLMovementSet, MovementSpeed);
	ATTRIBUTE_ACCESSORS(UNLMovementSet, MaxMovementSpeed);

	// Delegate when movement speed changes due to movement speed gameplay effect related updates.
	mutable FNLAttributeEvent OnMovementSpeedChanged;
	// Delegate when max movement speed changes due to max movement speed gameplay effect related updates.
	mutable FNLAttributeEvent OnMaxMovementSpeedChanged;
	
protected:

	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMovementSpeed(const FGameplayAttributeData& OldValue);
	
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current movement speed attribute.  The movement speed will be capped by the max movement speed attribute.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeed, Category = "NL|Movement", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MovementSpeed;

	// The current max movement speed attribute.  Max movement speed is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMovementSpeed, Category = "NL|Movement", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxMovementSpeed;

	// Store the movement speed before any changes 
	float MovementSpeedBeforeAttributeChange;
	float MaxMovementSpeedBeforeAttributeChange;
	
};
