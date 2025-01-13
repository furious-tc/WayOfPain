// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Pawns/NLHealthComponent.h"
#include "Abilities/Attributes/NLAttributeSet.h"
#include "System/NLAssetManager.h"
#include "System/NLGameData.h"
#include "NLGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Abilities/Attributes/NLHealthSet.h"
#include "Messages/NLVerbMessage.h"
#include "Messages/NLVerbMessageHelpers.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "NLLogChannels.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLHealthComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_NL_Elimination_Message, "NL.Elimination.Message");


UNLHealthComponent::UNLHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	EliminationState = ENLEliminationState::NotEliminated;
}

void UNLHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNLHealthComponent, EliminationState);
}

void UNLHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();

	Super::OnUnregister();
}

void UNLHealthComponent::InitializeWithAbilitySystem(UNLAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogNL, Error, TEXT("NLHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogNL, Error, TEXT("NLHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UNLHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogNL, Error, TEXT("NLHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	// Register to listen for attribute changes.
	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	// TEMP: Reset attributes to default values.  Eventually this will be driven by a spread sheet.
	AbilitySystemComponent->SetNumericAttributeBase(UNLHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());

	ClearGameplayTags();

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void UNLHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UNLHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(NLGameplayTags::Status_Elimination_BeingEliminated, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(NLGameplayTags::Status_Elimination_Eliminated, 0);
	}
}

float UNLHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float UNLHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float UNLHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();

		return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
	}

	return 0.0f;
}

void UNLHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UNLHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UNLHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		// Send the "GameplayEvent.Elimination" gameplay event through the owner's ability system.  This can be used to trigger a Elimination gameplay ability.
		{
			FGameplayEventData Payload;
			Payload.EventTag = NLGameplayTags::GameplayEvent_Elimination;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec->Def;
			Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		// Send a standardized verb message that other systems can observe
		{
			FNLVerbMessage Message;
			Message.Verb = TAG_NL_Elimination_Message;
			Message.Instigator = DamageInstigator;
			Message.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Message.Target = UNLVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
			Message.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			//@TODO: Fill out context tags, and any non-ability-system source/instigator tags
			//@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}

		//@TODO: assist messages (could compute from damage dealt elsewhere)?
	}

#endif // #if WITH_SERVER_CODE
}

void UNLHealthComponent::OnRep_EliminationState(ENLEliminationState OldEliminationState)
{
	const ENLEliminationState NewEliminationState = EliminationState;

	// Revert the Elimination state for now since we rely on StartElimination and FinishElimination to change it.
	EliminationState = OldEliminationState;

	if (OldEliminationState > NewEliminationState)
	{
		// The server is trying to set us back but we've already predicted past the server state.
		UE_LOG(LogNL, Warning, TEXT("NLHealthComponent: Predicted past server Elimination state [%d] -> [%d] for owner [%s]."), (uint8)OldEliminationState, (uint8)NewEliminationState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldEliminationState == ENLEliminationState::NotEliminated)
	{
		if (NewEliminationState == ENLEliminationState::EliminationStarted)
		{
			StartElimination();
		}
		else if (NewEliminationState == ENLEliminationState::EliminationFinished)
		{
			StartElimination();
			FinishElimination();
		}
		else
		{
			UE_LOG(LogNL, Error, TEXT("NLHealthComponent: Invalid Elimination transition [%d] -> [%d] for owner [%s]."), (uint8)OldEliminationState, (uint8)NewEliminationState, *GetNameSafe(GetOwner()));
		}
	}
	else if (OldEliminationState == ENLEliminationState::EliminationStarted)
	{
		if (NewEliminationState == ENLEliminationState::EliminationFinished)
		{
			FinishElimination();
		}
		else
		{
			UE_LOG(LogNL, Error, TEXT("NLHealthComponent: Invalid Elimination transition [%d] -> [%d] for owner [%s]."), (uint8)OldEliminationState, (uint8)NewEliminationState, *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf((EliminationState == NewEliminationState), TEXT("NLHealthComponent: Elimination transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldEliminationState, (uint8)NewEliminationState, *GetNameSafe(GetOwner()));
}

void UNLHealthComponent::StartElimination()
{
	if (EliminationState != ENLEliminationState::NotEliminated)
	{
		return;
	}

	EliminationState = ENLEliminationState::EliminationStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(NLGameplayTags::Status_Elimination_BeingEliminated, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnEliminationStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UNLHealthComponent::FinishElimination()
{
	if (EliminationState != ENLEliminationState::EliminationStarted)
	{
		return;
	}

	EliminationState = ENLEliminationState::EliminationFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(NLGameplayTags::Status_Elimination_Eliminated, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnEliminationFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UNLHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	if ((EliminationState == ENLEliminationState::NotEliminated) && AbilitySystemComponent)
	{
		const TSubclassOf<UGameplayEffect> DamageGE = UNLAssetManager::GetSubclass(UNLGameData::Get().DamageGameplayEffect_SetByCaller);
		if (!DamageGE)
		{
			UE_LOG(LogNL, Error, TEXT("NLHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to find gameplay effect [%s]."), *GetNameSafe(GetOwner()), *UNLGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		if (!Spec)
		{
			UE_LOG(LogNL, Error, TEXT("NLHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to make outgoing spec for [%s]."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGE));
			return;
		}

		Spec->AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);

		if (bFellOutOfWorld)
		{
			Spec->AddDynamicAssetTag(TAG_Gameplay_FellOutOfWorld);
		}

		const float DamageAmount = GetMaxHealth();

		Spec->SetSetByCallerMagnitude(NLGameplayTags::SetByCaller_Damage, DamageAmount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}

