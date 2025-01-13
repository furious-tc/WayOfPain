// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Pawns/NLPawnEffectManagerComponent.h"
#include "Player/NLPlayerState.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NLGameplayTags.h"
#include "Logging/MessageLog.h"
#include "NLLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPawnEffectManagerComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_NL_PawnEffect_Category, "NL.PawnEffect.Category");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_NL_PawnEffect_Message_StackChanged, "NL.PawnEffect.StackChanged.Message");

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

const FName UNLPawnEffectManagerComponent::NAME_ActorFeatureName("PawnEffect");

UNLPawnEffectManagerComponent::UNLPawnEffectManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
}

void UNLPawnEffectManagerComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogNL, Error, TEXT("[UNLPawnEffectManagerComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("NLPawnEffectManagerComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName NLMessageLogName = TEXT("NLPawnEffectManagerComponent");
			
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

bool UNLPawnEffectManagerComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

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
		// The player state is required.
		if (!GetPlayerState<ANLPlayerState>())
		{
			return false;
		}

		return true;
	}
	else if (CurrentState == NLGameplayTags::InitState_DataAvailable && DesiredState == NLGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		ANLPlayerState* NLPS = GetPlayerState<ANLPlayerState>();

		return NLPS && Manager->HasFeatureReachedInitState(Pawn, UNLPawnExtensionComponent::NAME_ActorFeatureName, NLGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == NLGameplayTags::InitState_DataInitialized && DesiredState == NLGameplayTags::InitState_GameplayReady)
	{
        UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
        
		if (!PawnExtComp)
		{
            return false;
		}

		return true;
	}

	return false;
}

void UNLPawnEffectManagerComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == NLGameplayTags::InitState_DataAvailable && DesiredState == NLGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ANLPlayerState* NLPS = GetPlayerState<ANLPlayerState>();
		if (!ensure(Pawn && NLPS))
		{
			return;
		}

		if (UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtComp->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
            PawnExtComp->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
		}
	}
}

void UNLPawnEffectManagerComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
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

void UNLPawnEffectManagerComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { NLGameplayTags::InitState_Spawned, NLGameplayTags::InitState_DataAvailable, NLGameplayTags::InitState_DataInitialized, NLGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UNLPawnEffectManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UNLPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(NLGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UNLPawnEffectManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UNLPawnEffectManagerComponent::OnAbilitySystemInitialized()
{
	APawn* Pawn = GetPawn<APawn>();

	if (!ensure(Pawn))
	{
		return;
	}

	UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
    check(PawnExtComp);

	NLASC = PawnExtComp->GetNLAbilitySystemComponent();
    
	if (!ensure(NLASC))
	{
        UE_LOG(LogNL, Error, TEXT("[UNLPawnEffectManagerComponent::OnAbilitySystemInitialized] This component is compatible with AbilitSystemComponent mixed replication mode."));
        return;
	}

	// Register to listen for duration based GameplayEffect changes.
    NLASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::OnActiveGameplayEffectAdded);
    NLASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::OnAnyGameplayEffectRemoved);
}

void UNLPawnEffectManagerComponent::OnAbilitySystemUninitialized()
{
	if (NLASC)
	{
        NLASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
        NLASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);

        NLASC = nullptr;
	}
}

void UNLPawnEffectManagerComponent::OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
    AActor* Owner = GetOwner();
    check(Owner);

	check(NLASC);

	FGameplayTagContainer AllAssetTags;
	SpecApplied.GetAllAssetTags(AllAssetTags);

	NLASC->OnGameplayEffectStackChangeDelegate(ActiveHandle)->AddUObject(this, &ThisClass::OnGameplayEffectStackChange);

	if (AllAssetTags.HasTagExact(TAG_NL_PawnEffect_Category))
	{
        UE_LOG(LogNL, Log, TEXT("UNLPawnEffectManagerComponent::OnActiveGameplayEffectAdded GE with Def [%s] added for pawn [%s]."), *GetPathNameSafe(SpecApplied.Def), *GetNameSafe(Owner));
		
		BroadcastChangeMessage(ActiveHandle, 0, SpecApplied.GetStackCount());
	}
}

void UNLPawnEffectManagerComponent::OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved)
{
	AActor* Owner = GetOwner();
	check(Owner);

	FGameplayTagContainer AllAssetTags;
	EffectRemoved.Spec.GetAllAssetTags(AllAssetTags);
	
	if (AllAssetTags.HasTagExact(TAG_NL_PawnEffect_Category))
	{
        UE_LOG(LogNL, Log, TEXT("UNLPawnEffectManagerComponent::OnAnyGameplayEffectRemoved GE with Def [%s] removed for owner [%s]."), *GetPathNameSafe(EffectRemoved.Spec.Def), *GetNameSafe(Owner));

		BroadcastChangeMessage(EffectRemoved.Handle, EffectRemoved.ClientCachedStackCount, 0);
	}
}

void UNLPawnEffectManagerComponent::OnGameplayEffectStackChange(FActiveGameplayEffectHandle EffectHandle, int32 NewStackCount, int32 PreviousStackCount)
{
	AActor* Owner = GetOwner();
	check(Owner);

	const FActiveGameplayEffect* ActiveGE = EffectHandle.GetOwningAbilitySystemComponent()->GetActiveGameplayEffects().GetActiveGameplayEffect(EffectHandle);

	FGameplayTagContainer AllAssetTags;
	ActiveGE->Spec.GetAllAssetTags(AllAssetTags);

	if (AllAssetTags.HasTagExact(TAG_NL_PawnEffect_Category))
	{
		UE_LOG(LogNL, Log, TEXT("UNLPawnEffectManagerComponent::OnGameplayEffectStackChange GE with Def [%s] stack count changed for owner [%s] from [%d] to [%d]."), *GetPathNameSafe(ActiveGE->Spec.Def), *GetNameSafe(Owner), PreviousStackCount, NewStackCount);

		BroadcastChangeMessage(EffectHandle, NewStackCount, PreviousStackCount);
	}
}

void UNLPawnEffectManagerComponent::BroadcastChangeMessage(FActiveGameplayEffectHandle EffectHandle, int32 NewStackCount, int32 PreviousStackCount)
{
    APawn* Pawn = GetPawn<APawn>();
    check(Pawn);

    FNLPawnEffectStackChangeMessage Message;
    Message.Player = Pawn;
    Message.EffectHandle = EffectHandle;
    Message.NewStackCount = NewStackCount;
    Message.PreviousStackCount = PreviousStackCount;
    Message.Delta = NewStackCount - PreviousStackCount;

    UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(Pawn->GetWorld());
    MessageSystem.BroadcastMessage(TAG_NL_PawnEffect_Message_StackChanged, Message);
}

TArray<FActiveGameplayEffectHandle> UNLPawnEffectManagerComponent::GetPawnGameplayEffectHandles() const
{
    return NLASC->GetActiveEffectsWithAllTags(FGameplayTagContainer(TAG_NL_PawnEffect_Category));
}
