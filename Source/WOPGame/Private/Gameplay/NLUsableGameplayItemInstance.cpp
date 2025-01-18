// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLUsableGameplayItemInstance.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputDeviceProperties.h"
#include "Pawns/NLHealthComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLUsableGameplayItemInstance)

class UAnimInstance;
struct FGameplayTagContainer;

UNLUsableGameplayItemInstance::UNLUsableGameplayItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Listen for elimination of the owning pawn so that any device properties can be removed if we're eliminated and can't unapply
	if (APawn* Pawn = GetPawn())
	{
		// We only need to do this for player controlled pawns, since AI and others won't have input devices on the client
		if (Pawn->IsPlayerControlled())
		{
			if (UNLHealthComponent* HealthComponent = UNLHealthComponent::FindHealthComponent(GetPawn()))
			{
				HealthComponent->OnEliminationStarted.AddDynamic(this, &ThisClass::OnEliminationStarted);
			}
		}
	}
}

void UNLUsableGameplayItemInstance::OnApplied()
{
	Super::OnApplied();

	UWorld* World = GetWorld();
	check(World);
	TimeLastApplied = World->GetTimeSeconds();

	ApplyDeviceProperties();
}

void UNLUsableGameplayItemInstance::OnUnapplied()
{
	Super::OnUnapplied();

	RemoveDeviceProperties();
}

void UNLUsableGameplayItemInstance::UpdateFiringTime()
{
	UWorld* World = GetWorld();
	check(World);
	TimeLastFired = World->GetTimeSeconds();
}

float UNLUsableGameplayItemInstance::GetTimeSinceLastInteractedWith() const
{
	UWorld* World = GetWorld();
	check(World);
	const double WorldTime = World->GetTimeSeconds();

	double Result = WorldTime - TimeLastApplied;

	if (TimeLastFired > 0.0)
	{
		const double TimeSinceFired = WorldTime - TimeLastFired;
		Result = FMath::Min(Result, TimeSinceFired);
	}

	return Result;
}

TSubclassOf<UAnimInstance> UNLUsableGameplayItemInstance::PickBestAnimLayer(bool bApplied, const FGameplayTagContainer& CosmeticTags) const
{
	const FNLAnimLayerSelectionSet& SetToQuery = (bApplied ? AppliedAnimSet : UnappliedAnimSet);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

const FPlatformUserId UNLUsableGameplayItemInstance::GetOwningUserId() const
{
	if (const APawn* Pawn = GetPawn())
	{
		return Pawn->GetPlatformUserId();
	}
	return PLATFORMUSERID_NONE;
}

void UNLUsableGameplayItemInstance::ApplyDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();

	if (UserId.IsValid())
	{
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			for (TObjectPtr<UInputDeviceProperty>& DeviceProp : ApplicableDeviceProperties)
			{
				FActivateDevicePropertyParams Params = {};
				Params.UserId = UserId;

				// By default, the device property will be played on the Platform User's Primary Input Device.
				// If you want to override this and set a specific device, then you can set the DeviceId parameter.
				//Params.DeviceId = <some specific device id>;
				
				// Don't remove this property it was evaluated. We want the properties to be applied as long as we are holding the 
				// usable gameplay item, and will remove them manually in OnUnapplied
				Params.bLooping = true;
			
				DevicePropertyHandles.Emplace(InputDeviceSubsystem->ActivateDeviceProperty(DeviceProp, Params));
			}
		}	
	}
}

void UNLUsableGameplayItemInstance::RemoveDeviceProperties()
{
	const FPlatformUserId UserId = GetOwningUserId();
	
	if (UserId.IsValid() && !DevicePropertyHandles.IsEmpty())
	{
		// Remove any device properties that have been applied
		if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
		{
			InputDeviceSubsystem->RemoveDevicePropertyHandles(DevicePropertyHandles);
			DevicePropertyHandles.Empty();
		}
	}
}

void UNLUsableGameplayItemInstance::OnEliminationStarted(AActor* OwningActor)
{
	// Remove any possibly active device properties when we die to make sure that there aren't any lingering around
	RemoveDeviceProperties();
}
