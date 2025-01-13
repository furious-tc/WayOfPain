// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Pawns/NLCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Abilities/Attributes/NLMovementSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "NLLogChannels.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLCharacterMovementComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");

UNLCharacterMovementComponent::UNLCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MovementSet = nullptr;
	InitialMaxWalkSpeed = MaxWalkSpeed;
}

void UNLCharacterMovementComponent::InitializeAbilitySystem(UNLAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogNL, Error, TEXT("NLCharacterMovementComponent: CharacterMovement component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogNL, Error, TEXT("NLCharacterMovementComponent: Cannot initialize CharacterMovement component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	MovementSet = AbilitySystemComponent->GetSet<UNLMovementSet>();
	if (!MovementSet)
	{
		UE_LOG(LogNL, Error, TEXT("NLCharacterMovementComponent: Cannot initialize CharacterMovement component for owner [%s] with NULL movement set on the ability system."), *GetNameSafe(Owner));
		return;
	}
	
	MovementSet->OnMovementSpeedChanged.AddUObject(this, &ThisClass::HandleMovementSpeedChanged_GAS);
}

void UNLCharacterMovementComponent::UninitializeAbilitySystem()
{
	if (MovementSet)
	{
		MovementSet->OnMovementSpeedChanged.RemoveAll(this);
		// @NLTODO: This is unlikely to occur but check if this also leads to stutter.
		MaxWalkSpeed = InitialMaxWalkSpeed;
	}

	MovementSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UNLCharacterMovementComponent::HandleMovementSpeedChanged_GAS(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
	if (CharacterOwner->IsLocallyControlled())
	{
		if (!CharacterOwner->HasAuthority())
		{
			Request_bPredictedMovementSetChange = 1;
		}
		else
		{
			MaxWalkSpeed = NewValue;
		}
	}
}

void UNLCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

void UNLCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UNLCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Request_bPredictedMovementSetChange = (Flags & FSavedMove_NL::FLAG_SetPredictedMovementSpeed) != 0;
}

void UNLCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (Request_bPredictedMovementSetChange)
	{
		if (MovementSet)
		{
			MaxWalkSpeed = MovementSet->GetMovementSpeed();
		}

		Request_bPredictedMovementSetChange = 0;
	}
}

void UNLCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

FRotator UNLCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	return Super::GetDeltaRotation(DeltaTime);
}

float UNLCharacterMovementComponent::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return 0;
		}
	}

	return Super::GetMaxSpeed();
}

FNetworkPredictionData_Client* UNLCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		UNLCharacterMovementComponent* MutableThis = const_cast<UNLCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_NL(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f; 
	}

	return ClientPredictionData;
}

UNLCharacterMovementComponent::FSavedMove_NL::FSavedMove_NL()
{
	Saved_bPredictedMovementSetChange = 0;
}

bool UNLCharacterMovementComponent::FSavedMove_NL::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_NL* NewNLMove = static_cast<FSavedMove_NL*>(NewMove.Get());

	if (Saved_bPredictedMovementSetChange != NewNLMove->Saved_bPredictedMovementSetChange)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UNLCharacterMovementComponent::FSavedMove_NL::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bPredictedMovementSetChange = 0;
}

uint8 UNLCharacterMovementComponent::FSavedMove_NL::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bPredictedMovementSetChange)
	{
		Result |= FLAG_SetPredictedMovementSpeed;
	}

	return Result;
}

void UNLCharacterMovementComponent::FSavedMove_NL::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	
	const UNLCharacterMovementComponent* CharacterMovement = Cast<UNLCharacterMovementComponent>(C->GetCharacterMovement());

	Saved_bPredictedMovementSetChange = CharacterMovement->Request_bPredictedMovementSetChange;
}

void UNLCharacterMovementComponent::FSavedMove_NL::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UNLCharacterMovementComponent* CharacterMovement = Cast<UNLCharacterMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Request_bPredictedMovementSetChange = Saved_bPredictedMovementSetChange;
}

UNLCharacterMovementComponent::FNetworkPredictionData_Client_NL::FNetworkPredictionData_Client_NL(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UNLCharacterMovementComponent::FNetworkPredictionData_Client_NL::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_NL());
}
