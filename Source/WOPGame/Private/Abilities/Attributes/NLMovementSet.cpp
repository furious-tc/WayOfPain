// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Abilities/Attributes/NLMovementSet.h"
#include "NLGameplayTags.h"
#include "Abilities/Attributes/NLAttributeSet.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayEffectAggregatorLibrary.h"
#include "Messages/NLVerbMessage.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLMovementSet)

UE_DEFINE_GAMEPLAY_TAG(TAG_NL_MovementSpeed_Message, "NL.MovementSpeed.Message");

UNLMovementSet::UNLMovementSet()
	: MovementSpeed(500.0f)
	, MaxMovementSpeed(1500.0f)
{
	MovementSpeedBeforeAttributeChange = 0.0f;
	MaxMovementSpeedBeforeAttributeChange = 0.0f;
}

void UNLMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UNLMovementSet, MovementSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNLMovementSet, MaxMovementSpeed, COND_None, REPNOTIFY_Always);
}

void UNLMovementSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNLMovementSet, MovementSpeed, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	// These events on the client should not be changing attributes

	const float CurrentMovementSpeed = GetMovementSpeed();
	const float CurrentMaxMovementSpeed = GetMaxMovementSpeed();
	const float EstimatedMagnitude = CurrentMovementSpeed - OldValue.GetCurrentValue();
	
	OnMovementSpeedChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentMovementSpeed);
}

void UNLMovementSet::OnRep_MaxMovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNLMovementSet, MaxMovementSpeed, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	OnMaxMovementSpeedChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxMovementSpeed() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxMovementSpeed());
}

bool UNLMovementSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// Save the current movement speed
	MovementSpeedBeforeAttributeChange = GetMovementSpeed();
	MaxMovementSpeedBeforeAttributeChange = GetMaxMovementSpeed();

	return true;
}

void UNLMovementSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetMovementSpeedAttribute())
	{
		// Send a standardized verb message that other systems can observe
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			FNLVerbMessage Message;
			Message.Verb = TAG_NL_MovementSpeed_Message;
			Message.Instigator = Data.EffectSpec.GetEffectContext().GetEffectCauser();
			Message.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			Message.Target = GetOwningActor();
			Message.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
			Message.Magnitude = Data.EvaluatedData.Magnitude;

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}
	}
}

void UNLMovementSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UNLMovementSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UNLMovementSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxMovementSpeedAttribute())
	{
		// Make sure current movement speed is within the allowed interval.
		if (GetMovementSpeed() > NewValue)
		{
			UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent();
			check(NLASC);

			NLASC->ApplyModToAttribute(GetMovementSpeedAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}

void UNLMovementSet::OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const
{
	Super::OnAttributeAggregatorCreated(Attribute, NewAggregator);

	if (!NewAggregator)
	{
		return;
	}

	if (Attribute == GetMovementSpeedAttribute())
	{
		NewAggregator->EvaluationMetaData = &FAggregatorEvaluateMetaDataLibrary::MostNegativeMod_AllPositiveMods;
	}
}

void UNLMovementSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	float MinimumMovementSpeed = 150.0f;

	if (Attribute == GetMovementSpeedAttribute())
	{
		// Make sure current movement speed is within the allowed interval.
		NewValue = FMath::Clamp(NewValue, MinimumMovementSpeed, GetMaxMovementSpeed());
	}
	else if (Attribute == GetMaxMovementSpeedAttribute())
	{
		// Do not allow max movement speed to drop below min movement speed.
		NewValue = FMath::Max(NewValue, MinimumMovementSpeed);
	}
}

