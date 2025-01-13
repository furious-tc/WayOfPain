// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#include "Abilities/NLAbilitySystemComponent.h"
#include "Abilities/NLGameplayAbility.h"
#include "Abilities/NLAbilityTagRelationshipMapping.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Abilities/NLGlobalAbilitySystem.h"
#include "NLLogChannels.h"
#include "System/NLAssetManager.h"
#include "System/NLGameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");


UNLAbilitySystemComponent::UNLAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
    InputHeldSpecHandles.Reset();

    FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UNLAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UNLGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UNLGlobalAbilitySystem>(GetWorld())) {
        GlobalAbilitySystem->UnregisterASC(this);
    }

    Super::EndPlay(EndPlayReason);
}

void UNLAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
    FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
    check(ActorInfo);
    check(InOwnerActor);

    const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

    Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

    if (bHasNewPawnAvatar) {
        // Notify all abilities that a new pawn avatar has been set
        for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
            ensureMsgf(AbilitySpec.Ability && AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("InitAbilityActorInfo: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

            TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();

            for (UGameplayAbility* AbilityInstance : Instances) {
                UNLGameplayAbility* NLAbilityInstance = Cast<UNLGameplayAbility>(AbilityInstance);
                if (NLAbilityInstance) {
                    // Ability instances may be missing for replays
                    NLAbilityInstance->OnPawnAvatarSet();
                }
            }
        }

        // Register with the global system once we actually have a pawn avatar. We wait until this time since some globally-applied effects may require an avatar.
        if (UNLGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UNLGlobalAbilitySystem>(GetWorld())) {
            GlobalAbilitySystem->RegisterASC(this);
        }

        TryActivateAbilitiesOnSpawn();
    }
}

void UNLAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {
        if (const UNLGameplayAbility* NLAbilityCDO = Cast<UNLGameplayAbility>(AbilitySpec.Ability)) {
            NLAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
        }
    }
}

void UNLAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility)
{
    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {
        if (!AbilitySpec.IsActive()) {
            continue;
        }

        UNLGameplayAbility* NLAbilityCDO = Cast<UNLGameplayAbility>(AbilitySpec.Ability);
        if (!NLAbilityCDO) {
            UE_LOG(LogNLAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Non-NLGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
            continue;
        }

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
            ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        // Cancel all the spawned instances, not the CDO.
        TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
        for (UGameplayAbility* AbilityInstance : Instances) {
            UNLGameplayAbility* NLAbilityInstance = CastChecked<UNLGameplayAbility>(AbilityInstance);

            if (ShouldCancelFunc(NLAbilityInstance, AbilitySpec.Handle)) {
                if (NLAbilityInstance->CanBeCanceled()) {
                    NLAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), NLAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
                } else {
                    UE_LOG(LogNLAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *NLAbilityInstance->GetName());
                }
            }
        }
    }
}

void UNLAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
    auto ShouldCancelFunc = [this](const UNLGameplayAbility* NLAbility, FGameplayAbilitySpecHandle Handle) {
        const ENLAbilityActivationPolicy ActivationPolicy = NLAbility->GetActivationPolicy();
        return ((ActivationPolicy == ENLAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy == ENLAbilityActivationPolicy::WhileInputActive));
    };

    CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UNLAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
    Super::AbilitySpecInputPressed(Spec);

    // We don't support UGameplayAbility::bReplicateInputDirectly.
    // Use replicated events instead so that the WaitInputPress ability task works.
    if (Spec.IsActive()) {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

        // Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
    }
}

void UNLAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
    Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

void UNLAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
    if (InputTag.IsValid()) {
        for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {
            if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))) {
                InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
                InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
            }
        }
    }
}

void UNLAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
    if (InputTag.IsValid()) {
        for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {
            if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))) {
                InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
                InputHeldSpecHandles.Remove(AbilitySpec.Handle);
            }
        }
    }
}

void UNLAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
    if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked)) {
        ClearAbilityInput();
        return;
    }

    static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
    AbilitiesToActivate.Reset();

    //@TODO: See if we can use FScopedServerAbilityRPCBatcher ScopedRPCBatcher in some of these loops

    //
    // Process all abilities that activate when the input is held.
    //
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles) {
        if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability && !AbilitySpec->IsActive()) {
                const UNLGameplayAbility* NLAbilityCDO = Cast<UNLGameplayAbility>(AbilitySpec->Ability);
                if (NLAbilityCDO && NLAbilityCDO->GetActivationPolicy() == ENLAbilityActivationPolicy::WhileInputActive) {
                    AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                }
            }
        }
    }

    //
    // Process all abilities that had their input pressed this frame.
    //
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles) {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability) {
                AbilitySpec->InputPressed = true;

                if (AbilitySpec->IsActive()) {
                    // Ability is active so pass along the input event.
                    AbilitySpecInputPressed(*AbilitySpec);
                } else {
                    const UNLGameplayAbility* NLAbilityCDO = Cast<UNLGameplayAbility>(AbilitySpec->Ability);

                    if (NLAbilityCDO && NLAbilityCDO->GetActivationPolicy() == ENLAbilityActivationPolicy::OnInputTriggered) {
                        AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                    }
                }
            }
        }
    }

    //
    // Try to activate all the abilities that are from presses and holds.
    // We do it all at once so that held inputs don't activate the ability
    // and then also send a input event to the ability because of the press.
    //
    for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate) {
        TryActivateAbility(AbilitySpecHandle);
    }

    //
    // Process all abilities that had their input released this frame.
    //
    for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles) {
        if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability) {
                AbilitySpec->InputPressed = false;

                if (AbilitySpec->IsActive()) {
                    // Ability is active so pass along the input event.
                    AbilitySpecInputReleased(*AbilitySpec);
                }
            }
        }
    }

    //
    // Clear the cached ability handles.
    //
    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
}

void UNLAbilitySystemComponent::ClearAbilityInput()
{
    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
    InputHeldSpecHandles.Reset();
}

void UNLAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
    Super::NotifyAbilityActivated(Handle, Ability);

    if (UNLGameplayAbility* NLAbility = Cast<UNLGameplayAbility>(Ability)) {
        AddAbilityToActivationGroup(NLAbility->GetActivationGroup(), NLAbility);
    }
}

void UNLAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
    Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

    if (APawn* Avatar = Cast<APawn>(GetAvatarActor())) {
        if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking()) {
            ClientNotifyAbilityFailed(Ability, FailureReason);
            return;
        }
    }

    HandleAbilityFailed(Ability, FailureReason);
}

void UNLAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
    Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

    if (UNLGameplayAbility* NLAbility = Cast<UNLGameplayAbility>(Ability)) {
        RemoveAbilityFromActivationGroup(NLAbility->GetActivationGroup(), NLAbility);
    }
}

void UNLAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
    FGameplayTagContainer ModifiedBlockTags = BlockTags;
    FGameplayTagContainer ModifiedCancelTags = CancelTags;

    if (TagRelationshipMapping) {
        // Use the mapping to expand the ability tags into block and cancel tag
        TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
    }

    Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags, bExecuteCancelTags, ModifiedCancelTags);

    //@TODO: Apply any special logic like blocking input or movement
}

void UNLAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
    Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

    //@TODO: Apply any special logic like blocking input or movement
}

void UNLAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
{
    if (TagRelationshipMapping) {
        TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
    }
}

void UNLAbilitySystemComponent::SetTagRelationshipMapping(UNLAbilityTagRelationshipMapping* NewMapping)
{
    TagRelationshipMapping = NewMapping;
}

void UNLAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
    HandleAbilityFailed(Ability, FailureReason);
}

void UNLAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
    // UE_LOG(LogNLAbilitySystem, Warning, TEXT("Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

    if (const UNLGameplayAbility* NLAbility = Cast<const UNLGameplayAbility>(Ability)) {
        NLAbility->OnAbilityFailedToActivate(FailureReason);
    }
}

bool UNLAbilitySystemComponent::IsActivationGroupBlocked(ENLAbilityActivationGroup Group) const
{
    bool bBlocked = false;

    switch (Group) {
    case ENLAbilityActivationGroup::Independent:
        // Independent abilities are never blocked.
        bBlocked = false;
        break;

    case ENLAbilityActivationGroup::Exclusive_Replaceable:
    case ENLAbilityActivationGroup::Exclusive_Blocking:
        // Exclusive abilities can activate if nothing is blocking.
        bBlocked = (ActivationGroupCounts[(uint8)ENLAbilityActivationGroup::Exclusive_Blocking] > 0);
        break;

    default:
        checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
        break;
    }

    return bBlocked;
}

void UNLAbilitySystemComponent::AddAbilityToActivationGroup(ENLAbilityActivationGroup Group, UNLGameplayAbility* NLAbility)
{
    check(NLAbility);
    check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);

    ActivationGroupCounts[(uint8)Group]++;

    const bool bReplicateCancelAbility = false;

    switch (Group) {
    case ENLAbilityActivationGroup::Independent:
        // Independent abilities do not cancel any other abilities.
        break;

    case ENLAbilityActivationGroup::Exclusive_Replaceable:
    case ENLAbilityActivationGroup::Exclusive_Blocking:
        CancelActivationGroupAbilities(ENLAbilityActivationGroup::Exclusive_Replaceable, NLAbility, bReplicateCancelAbility);
        break;

    default:
        checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
        break;
    }

    const int32 ExclusiveCount = ActivationGroupCounts[(uint8)ENLAbilityActivationGroup::Exclusive_Replaceable] + ActivationGroupCounts[(uint8)ENLAbilityActivationGroup::Exclusive_Blocking];
    if (!ensure(ExclusiveCount <= 1)) {
        UE_LOG(LogNLAbilitySystem, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
    }
}

void UNLAbilitySystemComponent::RemoveAbilityFromActivationGroup(ENLAbilityActivationGroup Group, UNLGameplayAbility* NLAbility)
{
    check(NLAbility);
    check(ActivationGroupCounts[(uint8)Group] > 0);

    ActivationGroupCounts[(uint8)Group]--;
}

void UNLAbilitySystemComponent::CancelActivationGroupAbilities(ENLAbilityActivationGroup Group, UNLGameplayAbility* IgnoreNLAbility, bool bReplicateCancelAbility)
{
    auto ShouldCancelFunc = [this, Group, IgnoreNLAbility](const UNLGameplayAbility* NLAbility, FGameplayAbilitySpecHandle Handle) {
        return ((NLAbility->GetActivationGroup() == Group) && (NLAbility != IgnoreNLAbility));
    };

    CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UNLAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
    const TSubclassOf<UGameplayEffect> DynamicTagGE = UNLAssetManager::GetSubclass(UNLGameData::Get().DynamicTagGameplayEffect);
    if (!DynamicTagGE) {
        UE_LOG(LogNLAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UNLGameData::Get().DynamicTagGameplayEffect.GetAssetName());
        return;
    }

    const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
    FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

    if (!Spec) {
        UE_LOG(LogNLAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
        return;
    }

    Spec->DynamicGrantedTags.AddTag(Tag);

    ApplyGameplayEffectSpecToSelf(*Spec);
}

void UNLAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
    const TSubclassOf<UGameplayEffect> DynamicTagGE = UNLAssetManager::GetSubclass(UNLGameData::Get().DynamicTagGameplayEffect);
    if (!DynamicTagGE) {
        UE_LOG(LogNLAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *UNLGameData::Get().DynamicTagGameplayEffect.GetAssetName());
        return;
    }

    FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
    Query.EffectDefinition = DynamicTagGE;

    RemoveActiveEffects(Query);
}

void UNLAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
    TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
    if (ReplicatedData.IsValid()) {
        OutTargetDataHandle = ReplicatedData->TargetData;
    }
}
