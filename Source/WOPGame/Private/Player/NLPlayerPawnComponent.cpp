// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Player/NLPlayerPawnComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Logging/MessageLog.h"
#include "NLLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "Player/NLPlayerState.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "Pawns/NLPawnData.h"
#include "Pawns/NLCharacter.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Input/NLInputConfig.h"
#include "Input/NLInputComponent.h"
#include "NLGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "AbilitySystemGlobals.h"
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPlayerPawnComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace NLLegendPlayer
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UNLPlayerPawnComponent::NAME_BindInputsNow("BindInputsNow");
const FName UNLPlayerPawnComponent::NAME_ActorFeatureName("PlayerPawn");

UNLPlayerPawnComponent::UNLPlayerPawnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReadyToBindInputs = false;
}

void UNLPlayerPawnComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogNL, Error, TEXT("[UNLPlayerPawnComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("NLPlayerPawnComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName NLMessageLogName = TEXT("NLPlayerPawnComponent");
			
			FMessageLog(NLMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
				
			FMessageLog(NLMessageLogName).Open();
		}
#endif
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool UNLPlayerPawnComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
    AController* Controller = GetController<AController>();
	UNLAbilitySystemComponent* NLASC = Cast<UNLAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));

	if (!CurrentState.IsValid() && DesiredState == NLGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == NLGameplayTags::InitState_Spawned && DesiredState == NLGameplayTags::InitState_DataAvailable)
	{
		if (!Controller)
		{
            return false;
		}

		if (!NLASC)
		{
			// The player state is required for non-bot pawns.
			if (!GetPlayerState<ANLPlayerState>())
			{
				return false;
			}

			// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
			if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
			{
				const bool bHasControllerPairedWithPS = (Controller->PlayerState != nullptr) && (Controller->PlayerState->GetOwner() == Controller);

				if (!bHasControllerPairedWithPS)
				{
					return false;
				}
			}
        }

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = !Controller->IsPlayerController();

		if (bIsLocallyControlled && !bIsBot)
		{
			APlayerController* PC = GetController<APlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !PC || !PC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == NLGameplayTags::InitState_DataAvailable && DesiredState == NLGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		ANLPlayerState* NLPS = GetPlayerState<ANLPlayerState>();

		const bool bIsBot = Controller && !Controller->IsPlayerController();

		return (NLPS || bIsBot) && Manager->HasFeatureReachedInitState(Pawn, UNLPawnExtensionComponent::NAME_ActorFeatureName, NLGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == NLGameplayTags::InitState_DataInitialized && DesiredState == NLGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UNLPlayerPawnComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == NLGameplayTags::InitState_DataAvailable && DesiredState == NLGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AController* Controller = GetController<AController>();
		UNLAbilitySystemComponent* NLASC = Cast<UNLAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn));

		ANLPlayerState* NLPS = GetPlayerState<ANLPlayerState>();
		if (!ensure(Pawn && (NLPS || NLASC)))
		{
			return;
		}

		const UNLPawnData* PawnData = nullptr;

		if (UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UNLPawnData>();

			// The player state holds the persistent data for this player (state that persists across eliminations and multiple pawns).
			// The ability system component and attribute sets live on the player state.

			if (NLPS)
			{
                PawnExtComp->InitializeAbilitySystem(NLPS->GetNLAbilitySystemComponent(), NLPS);
			}
			else
			{
                PawnExtComp->InitializeAbilitySystem(NLASC, Pawn);
			}
		}

		if (APlayerController* PC = GetController<APlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}
	}
}

void UNLPlayerPawnComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UNLPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == NLGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UNLPlayerPawnComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { NLGameplayTags::InitState_Spawned, NLGameplayTags::InitState_DataAvailable, NLGameplayTags::InitState_DataInitialized, NLGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UNLPlayerPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UNLPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(NLGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UNLPlayerPawnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UNLPlayerPawnComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (bClearExistingMappingsOnInit)
	{
        Subsystem->ClearAllMappings();
	}

	if (const UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UNLPawnData* PawnData = PawnExtComp->GetPawnData<UNLPawnData>())
		{
			for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
			{
				if (UInputMappingContext* IMC = Mapping.InputMapping.Get())
				{
					if (Mapping.bRegisterWithSettings)
					{
						if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
						{
							Settings->RegisterInputMappingContext(IMC);
						}
							
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;
						// Actually add the config to the local player							
						Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
					}
				}
			}

			if (const UNLInputConfig* InputConfig = PawnData->InputConfig)
			{
				// The NL Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the UNLInputComponent or modify this component accordingly.
				UNLInputComponent* NLIC = Cast<UNLInputComponent>(PlayerInputComponent);
				if (ensureMsgf(NLIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UNLInputComponent or a subclass of it.")))
				{
					// Add the key mappings that may have been set by the player
					NLIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events. 
					TArray<uint32> BindHandles;
					NLIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
				
					NLIC->BindNativeAction(InputConfig, NLGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					NLIC->BindNativeAction(InputConfig, NLGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					NLIC->BindNativeAction(InputConfig, NLGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}
 
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UNLPlayerPawnComponent::AddAdditionalInputConfig(const UNLInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}
	
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UNLInputComponent* NLIC = Pawn->FindComponentByClass<UNLInputComponent>();
		if (ensureMsgf(NLIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UNLInputComponent or a subclass of it.")))
		{
			NLIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UNLPlayerPawnComponent::RemoveAdditionalInputConfig(const UNLInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UNLPlayerPawnComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UNLPlayerPawnComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UNLAbilitySystemComponent* NLASC = PawnExtComp->GetNLAbilitySystemComponent())
			{
				NLASC->AbilityInputTagPressed(InputTag);
			}
		}	
	}
}

void UNLPlayerPawnComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UNLAbilitySystemComponent* NLASC = PawnExtComp->GetNLAbilitySystemComponent())
		{
			NLASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UNLPlayerPawnComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		FVector2D ScaleVector = FVector2D(1);

		if (const ANLPlayerController* NLPC = Cast<ANLPlayerController>(Controller))
		{
			ScaleVector = NLPC->GetMovementInputScaleValue(Value);
		}

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UNLPlayerPawnComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UNLPlayerPawnComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * NLLegendPlayer::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * NLLegendPlayer::LookPitchRate * World->GetDeltaSeconds());
	}
}
