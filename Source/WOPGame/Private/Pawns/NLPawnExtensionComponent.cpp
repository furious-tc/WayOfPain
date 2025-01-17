// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#include "Pawns/NLPawnExtensionComponent.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "NLGameplayTags.h"
#include "NLLogChannels.h"
#include "Pawns/NLPawnData.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPawnExtensionComponent)

class UActorComponent;

const FName UNLPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UNLPawnExtensionComponent::UNLPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    AbilitySystemComponent = nullptr;
}

void UNLPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNLPawnExtensionComponent, PawnData);
}

void UNLPawnExtensionComponent::OnRegister()
{
    Super::OnRegister();

    const APawn* Pawn = GetPawn<APawn>();
    ensureAlwaysMsgf((Pawn != nullptr), TEXT("NLPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

    TArray<UActorComponent*> PawnExtensionComponents;
    Pawn->GetComponents(UNLPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
    ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one NLPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

    // Register with the init state system early, this will only work if this is a game world
    RegisterInitStateFeature();
}

void UNLPawnExtensionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Listen for changes to all features
    BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

    // Notifies state manager that we have spawned, then try rest of default initialization
    ensure(TryToChangeInitState(NLGameplayTags::InitState_Spawned));
    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UninitializeAbilitySystem();
    UnregisterInitStateFeature();

    Super::EndPlay(EndPlayReason);
}

void UNLPawnExtensionComponent::SetPawnData(const UNLPawnData* InPawnData)
{
    check(InPawnData);

    APawn* Pawn = GetPawnChecked<APawn>();

    if (Pawn->GetLocalRole() != ROLE_Authority) {
        return;
    }

    if (PawnData) {
        UE_LOG(LogNL, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
        return;
    }

    PawnData = InPawnData;

    Pawn->ForceNetUpdate();

    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::OnRep_PawnData()
{
    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::InitializeAbilitySystem(UNLAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
    check(InASC);
    check(InOwnerActor);

    if (AbilitySystemComponent == InASC) {
        // The ability system component hasn't changed.
        return;
    }

    if (AbilitySystemComponent) {
        // Clean up the old ability system component.
        UninitializeAbilitySystem();
    }

    APawn* Pawn = GetPawnChecked<APawn>();
    AActor* ExistingAvatar = InASC->GetAvatarActor();

    UE_LOG(LogNL, Verbose, TEXT("Setting up ASC [%s] on pawn [%s] owner [%s], existing [%s] "), *GetNameSafe(InASC), *GetNameSafe(Pawn), *GetNameSafe(InOwnerActor), *GetNameSafe(ExistingAvatar));

    if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn)) {
        UE_LOG(LogNL, Log, TEXT("Existing avatar (authority=%d)"), ExistingAvatar->HasAuthority() ? 1 : 0);

        // There is already a pawn acting as the ASC's avatar, so we need to kick it out
        // This can happen on clients if they're lagged: their new pawn is spawned + possessed before the elimination one is removed
        ensure(!ExistingAvatar->HasAuthority());

        if (UNLPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar)) {
            OtherExtensionComponent->UninitializeAbilitySystem();
        }
    }

    AbilitySystemComponent = InASC;
    AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

    if (ensure(PawnData)) {
        InASC->SetTagRelationshipMapping(PawnData->TagRelationshipMapping);
    }

    OnAbilitySystemInitialized.Broadcast();
}

void UNLPawnExtensionComponent::UninitializeAbilitySystem()
{
    if (!AbilitySystemComponent) {
        return;
    }

    // Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
    if (AbilitySystemComponent->GetAvatarActor() == GetOwner()) {
        FGameplayTagContainer AbilityTypesToIgnore;
        AbilityTypesToIgnore.AddTag(NLGameplayTags::Ability_Behavior_SurvivesElimination);

        AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
        AbilitySystemComponent->ClearAbilityInput();
        AbilitySystemComponent->RemoveAllGameplayCues();

        if (AbilitySystemComponent->GetOwnerActor() != nullptr) {
            AbilitySystemComponent->SetAvatarActor(nullptr);
        } else {
            // If the ASC doesn't have a valid owner, we need to clear *all* actor info, not just the avatar pairing
            AbilitySystemComponent->ClearActorInfo();
        }

        OnAbilitySystemUninitialized.Broadcast();
    }

    AbilitySystemComponent = nullptr;
}

void UNLPawnExtensionComponent::HandleControllerChanged()
{
    if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>())) {
        ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
        if (AbilitySystemComponent->GetOwnerActor() == nullptr) {
            UninitializeAbilitySystem();
        } else {
            AbilitySystemComponent->RefreshAbilityActorInfo();
        }
    }

    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::HandlePlayerStateReplicated()
{
    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::SetupPlayerInputComponent()
{
    CheckDefaultInitialization();
}

void UNLPawnExtensionComponent::CheckDefaultInitialization()
{
    // Before checking our progress, try progressing any other features we might depend on
    CheckDefaultInitializationForImplementers();

    static const TArray<FGameplayTag> StateChain = { NLGameplayTags::InitState_Spawned, NLGameplayTags::InitState_DataAvailable, NLGameplayTags::InitState_DataInitialized, NLGameplayTags::InitState_GameplayReady };

    // This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
    ContinueInitStateChain(StateChain);
}

bool UNLPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
    check(Manager);

    APawn* Pawn = GetPawn<APawn>();
    if (!CurrentState.IsValid() && DesiredState == NLGameplayTags::InitState_Spawned) {
        // As long as we are on a valid pawn, we count as spawned
        if (Pawn) {
            return true;
        }
    }
    if (CurrentState == NLGameplayTags::InitState_Spawned && DesiredState == NLGameplayTags::InitState_DataAvailable) {
        // Pawn data is required.
        if (!PawnData) {
            return false;
        }

        const bool bHasAuthority = Pawn->HasAuthority();
        const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
        const bool bIsBotControlled = Pawn->IsBotControlled();

        if (bHasAuthority || bIsLocallyControlled || bIsBotControlled) {
            // Check for being possessed by a controller.
            if (!GetController<AController>()) {
                return false;
            }
        }

        return true;
    } else if (CurrentState == NLGameplayTags::InitState_DataAvailable && DesiredState == NLGameplayTags::InitState_DataInitialized) {
        // Transition to initialize if all features have their data available
        return Manager->HaveAllFeaturesReachedInitState(Pawn, NLGameplayTags::InitState_DataAvailable);
    } else if (CurrentState == NLGameplayTags::InitState_DataInitialized && DesiredState == NLGameplayTags::InitState_GameplayReady) {
        return true;
    }

    return false;
}

void UNLPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
    if (DesiredState == NLGameplayTags::InitState_DataInitialized) {
        // This is currently all handled by other components listening to this state change
    }
}

void UNLPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
    // If another feature is now in DataAvailable, see if we should transition to DataInitialized
    if (Params.FeatureName != NAME_ActorFeatureName) {
        if (Params.FeatureState == NLGameplayTags::InitState_DataAvailable) {
            CheckDefaultInitialization();
        }
    }
}

void UNLPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
    if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject())) {
        OnAbilitySystemInitialized.Add(Delegate);
    }

    if (AbilitySystemComponent) {
        Delegate.Execute();
    }
}

void UNLPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
    if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject())) {
        OnAbilitySystemUninitialized.Add(Delegate);
    }
}
