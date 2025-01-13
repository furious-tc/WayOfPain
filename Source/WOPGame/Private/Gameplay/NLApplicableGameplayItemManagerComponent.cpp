// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLApplicableGameplayItemManagerComponent.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/ActorChannel.h"
#include "Gameplay/NLApplicableGameplayItemDefinition.h"
#include "Gameplay/NLApplicableGameplayItemInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLApplicableGameplayItemManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

//////////////////////////////////////////////////////////////////////
// FNLAppliedGameplayItemEntry

FString FNLAppliedGameplayItemEntry::GetDebugString() const
{
    return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(ApplicableGameplayItemDefinition.Get()));
}

//////////////////////////////////////////////////////////////////////
// FNLAppliedGameplayItemList

void FNLAppliedGameplayItemList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    for (int32 Index : RemovedIndices) {
        const FNLAppliedGameplayItemEntry& Entry = Entries[Index];
        if (Entry.Instance != nullptr) {
            Entry.Instance->OnUnapplied();
        }
    }
}

void FNLAppliedGameplayItemList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    for (int32 Index : AddedIndices) {
        const FNLAppliedGameplayItemEntry& Entry = Entries[Index];
        if (Entry.Instance != nullptr) {
            Entry.Instance->OnApplied();
        }
    }
}

void FNLAppliedGameplayItemList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    // 	for (int32 Index : ChangedIndices)
    // 	{
    // 		const FGameplayTagStack& Stack = Stacks[Index];
    // 		TagToCountMap[Stack.Tag] = Stack.StackCount;
    // 	}
}

UNLAbilitySystemComponent* FNLAppliedGameplayItemList::GetAbilitySystemComponent() const
{
    check(OwnerComponent);
    AActor* OwningActor = OwnerComponent->GetOwner();
    return Cast<UNLAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UNLApplicableGameplayItemInstance* FNLAppliedGameplayItemList::AddEntry(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition)
{
    UNLApplicableGameplayItemInstance* Result = nullptr;

    check(ApplicableGameplayItemDefinition != nullptr);
    check(OwnerComponent);
    check(OwnerComponent->GetOwner()->HasAuthority());

    const UNLApplicableGameplayItemDefinition* ApplicableGameplayItemCDO = GetDefault<UNLApplicableGameplayItemDefinition>(ApplicableGameplayItemDefinition);

    TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType = ApplicableGameplayItemCDO->InstanceType;
    if (InstanceType == nullptr) {
        InstanceType = UNLApplicableGameplayItemInstance::StaticClass();
    }

    FNLAppliedGameplayItemEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.ApplicableGameplayItemDefinition = ApplicableGameplayItemDefinition;
    NewEntry.Instance = NewObject<UNLApplicableGameplayItemInstance>(OwnerComponent->GetOwner(), InstanceType); //@TODO: Using the actor instead of component as the outer due to UE-127172
    NewEntry.Instance->SetApplicableItemDef(ApplicableGameplayItemDefinition);
    Result = NewEntry.Instance;

    if (UNLAbilitySystemComponent* ASC = GetAbilitySystemComponent()) {
        for (const TObjectPtr<const UNLAbilitySet>& AbilitySet : ApplicableGameplayItemCDO->AbilitySetsToGrant) {
            AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, Result);
        }
    } else {
        //@TODO: Warning logging?
    }

    if (!ApplicableGameplayItemCDO->bManagedSpawn)
    {
        Result->SpawnApplicableGameplayItemActors();
    }

    MarkItemDirty(NewEntry);

    return Result;
}

void FNLAppliedGameplayItemList::RemoveEntry(UNLApplicableGameplayItemInstance* Instance)
{
    for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt) {
        FNLAppliedGameplayItemEntry& Entry = *EntryIt;
        if (Entry.Instance == Instance) {
            if (UNLAbilitySystemComponent* ASC = GetAbilitySystemComponent()) {
                Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
            }

            Instance->DestroyApplicableGameplayItemActors();

            EntryIt.RemoveCurrent();
            MarkArrayDirty();
        }
    }
}

//////////////////////////////////////////////////////////////////////
// UNLApplicableGameplayItemManagerComponent

UNLApplicableGameplayItemManagerComponent::UNLApplicableGameplayItemManagerComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , AppliedGameplayItemList(this)
{
    SetIsReplicatedByDefault(true);
    bWantsInitializeComponent = true;
}

void UNLApplicableGameplayItemManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, AppliedGameplayItemList);
}

UNLApplicableGameplayItemInstance* UNLApplicableGameplayItemManagerComponent::ApplyItem(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemClass, UObject* Instigator)
{
    UNLApplicableGameplayItemInstance* Result = nullptr;
    if (ApplicableGameplayItemClass != nullptr) {
        Result = AppliedGameplayItemList.AddEntry(ApplicableGameplayItemClass);
        if (Result != nullptr) {
            Result->SetInstigator(Instigator);
            Result->OnApplied();

            if (IsUsingRegisteredSubObjectList() && IsReadyForReplication()) {
                AddReplicatedSubObject(Result);
            }
        }
    }
    return Result;
}

void UNLApplicableGameplayItemManagerComponent::UnapplyItem(UNLApplicableGameplayItemInstance* ItemInstance)
{
    if (ItemInstance != nullptr) {
        if (IsUsingRegisteredSubObjectList()) {
            RemoveReplicatedSubObject(ItemInstance);
        }

        ItemInstance->OnUnapplied();
        AppliedGameplayItemList.RemoveEntry(ItemInstance);
    }
}

void UNLApplicableGameplayItemManagerComponent::UnapplyItemsByDefinition(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition)
{
    TArray<UNLApplicableGameplayItemInstance*> ApplicableGameplayItemInstancesByDefinition;

    // Gathering instances of given definition before removal to avoid side effects affecting the ApplicableGameplayItem list iterator
    for (const FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        if (Entry.ApplicableGameplayItemDefinition == ApplicableGameplayItemDefinition)
        {
            ApplicableGameplayItemInstancesByDefinition.Add(Entry.Instance);
        }
    }

    for (UNLApplicableGameplayItemInstance* ApplicableItemInstance : ApplicableGameplayItemInstancesByDefinition) {
        UnapplyItem(ApplicableItemInstance);
    }
}
bool UNLApplicableGameplayItemManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    for (FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        UNLApplicableGameplayItemInstance* Instance = Entry.Instance;

        if (IsValid(Instance)) {
            WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
        }
    }

    return WroteSomething;
}

void UNLApplicableGameplayItemManagerComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void UNLApplicableGameplayItemManagerComponent::UninitializeComponent()
{
    TArray<UNLApplicableGameplayItemInstance*> AllApplicableGameplayItemInstances;

    // gathering all instances before removal to avoid side effects affecting the ApplicableGameplayItem list iterator
    for (const FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        AllApplicableGameplayItemInstances.Add(Entry.Instance);
    }

    for (UNLApplicableGameplayItemInstance* EquipInstance : AllApplicableGameplayItemInstances) {
        UnapplyItem(EquipInstance);
    }

    Super::UninitializeComponent();
}

void UNLApplicableGameplayItemManagerComponent::ReadyForReplication()
{
    Super::ReadyForReplication();

    // Register existing NLApplicableGameplayItemInstances
    if (IsUsingRegisteredSubObjectList()) {
        for (const FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
            UNLApplicableGameplayItemInstance* Instance = Entry.Instance;

            if (IsValid(Instance)) {
                AddReplicatedSubObject(Instance);
            }
        }
    }
}

UNLApplicableGameplayItemInstance* UNLApplicableGameplayItemManagerComponent::GetFirstInstanceOfType(TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType)
{
    for (FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        if (UNLApplicableGameplayItemInstance* Instance = Entry.Instance) {
            if (Instance->IsA(InstanceType)) {
                return Instance;
            }
        }
    }

    return nullptr;
}

TArray<UNLApplicableGameplayItemInstance*> UNLApplicableGameplayItemManagerComponent::GetApplicableGameplayItemInstancesOfType(TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType) const
{
    TArray<UNLApplicableGameplayItemInstance*> Results;
    for (const FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        if (UNLApplicableGameplayItemInstance* Instance = Entry.Instance) {
            if (Instance->IsA(InstanceType)) {
                Results.Add(Instance);
            }
        }
    }
    return Results;
}

TArray<UNLApplicableGameplayItemInstance*> UNLApplicableGameplayItemManagerComponent::GetApplicableGameplayItemInstancesOfDefinition(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableItemDefinition) const
{
    TArray<UNLApplicableGameplayItemInstance*> Results;
    for (const FNLAppliedGameplayItemEntry& Entry : AppliedGameplayItemList.Entries) {
        if (ApplicableItemDefinition == Entry.ApplicableGameplayItemDefinition)
        {
            Results.Add(Entry.Instance);
        }
    }

    return Results;
}
