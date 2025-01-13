// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLGameplayItemManagerComponent.h"
#include "Gameplay/NLGameplayItemDefinition.h"
#include "Gameplay/NLGameplayItemInstance.h"
#include "Gameplay/NLGameplaySubsystem.h"
#include "Gameplay/NLApplicableGameplayItemManagerComponent.h"
#include "Gameplay/NLGameplayItemFragment_ApplicableGameplayItemDefinition.h"
#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameplayItemManagerComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_NL_GameplayItem_Message_StackChanged, "NL.GameplayItem.StackChanged.Message");

//////////////////////////////////////////////////////////////////////
// FNLGameplayItemEntry

FString FNLGameplayItemEntry::GetDebugString() const
{
	TSubclassOf<UNLGameplayItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FNLGameplayItemList

void FNLGameplayItemList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FNLGameplayItemEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.StackCount, /*NewCount=*/ 0);
		Stack.LastObservedCount = 0;
	}
}

void FNLGameplayItemList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FNLGameplayItemEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ 0, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FNLGameplayItemList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FNLGameplayItemEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.LastObservedCount, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FNLGameplayItemList::BroadcastChangeMessage(FNLGameplayItemEntry& Entry, int32 OldCount, int32 NewCount)
{
	FNLGameplayItemChangeMessage Message;
	Message.GameplayItemOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_NL_GameplayItem_Message_StackChanged, Message);
}

UNLGameplayItemInstance* FNLGameplayItemList::AddEntry(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 StackCount)
{
	UNLGameplayItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
 	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FNLGameplayItemEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UNLGameplayItemInstance>(OwnerComponent->GetOwner());  //@TODO: Using the actor instead of component as the outer due to UE-127172
	NewEntry.Instance->SetItemDef(ItemDef);

	for (UNLGameplayItemFragment* Fragment : GetDefault<UNLGameplayItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	MarkItemDirty(NewEntry);

    BroadcastChangeMessage(NewEntry, /*OldCount=*/0, /*NewCount=*/NewEntry.StackCount);
    NewEntry.LastObservedCount = NewEntry.StackCount;

	return Result;
}

void FNLGameplayItemList::RemoveEntry(UNLGameplayItemInstance* Instance)
{
    AActor* OwningActor = OwnerComponent->GetOwner();
    check(OwningActor->HasAuthority());

	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FNLGameplayItemEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();

            BroadcastChangeMessage(Entry, /*OldCount=*/Entry.StackCount, /*NewCount=*/0);
            Entry.LastObservedCount = 0;

			MarkArrayDirty();
		}
	}
}

TArray<UNLGameplayItemInstance*> FNLGameplayItemList::GetAllItems() const
{
	TArray<UNLGameplayItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FNLGameplayItemEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

//////////////////////////////////////////////////////////////////////
// UNLGameplayItemManagerComponent

UNLGameplayItemManagerComponent::UNLGameplayItemManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GameplayItemList(this)
{
	SetIsReplicatedByDefault(true);
}

void UNLGameplayItemManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, GameplayItemList);
}

bool UNLGameplayItemManagerComponent::CanAddItemDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 StackCount)
{
    UNLGameplaySubsystem* NLGameplaySubsystem = UWorld::GetSubsystem<UNLGameplaySubsystem>(GetOwner()->GetWorld());

    if (const UNLGameplayItemFragment_ApplicableGameplayItemDefinition* ApplicableGameplayItemDefinitionFragment = Cast<UNLGameplayItemFragment_ApplicableGameplayItemDefinition>(NLGameplaySubsystem->FindItemDefinitionFragment(ItemDef, UNLGameplayItemFragment_ApplicableGameplayItemDefinition::StaticClass())))
	{	
		const bool bIsUniqueCategory = ApplicableGameplayItemDefinitionFragment->bUniqueCategory;

        if (!bIsUniqueCategory && GetTotalItemCountByDefinition(ItemDef) + StackCount > ApplicableGameplayItemDefinitionFragment->StackLimit) {
            return false;
        }
    }

	return true;
}

UNLGameplayItemInstance* UNLGameplayItemManagerComponent::AddItemDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 StackCount)
{
	UNLGameplayItemInstance* Result = nullptr;

    if (ItemDef == nullptr || !CanAddItemDefinition(ItemDef, StackCount)) {
        return Result;
    }

	UNLGameplaySubsystem* NLGameplaySubsystem = UWorld::GetSubsystem<UNLGameplaySubsystem>(GetOwner()->GetWorld());

	UNLApplicableGameplayItemManagerComponent* ApplicableGameplayItemManager = FindApplicableGameplayItemManager();

	check(ApplicableGameplayItemManager);

	bool bApplyGameplayItemOnGather = false;
	const UNLGameplayItemFragment_ApplicableGameplayItemDefinition* ApplicableGameplayItemDefinitionFragment = Cast<UNLGameplayItemFragment_ApplicableGameplayItemDefinition>(NLGameplaySubsystem->FindItemDefinitionFragment(ItemDef, UNLGameplayItemFragment_ApplicableGameplayItemDefinition::StaticClass()));

	if (ApplicableGameplayItemDefinitionFragment)
	{
        bApplyGameplayItemOnGather = ApplicableGameplayItemDefinitionFragment->bApplyGameplayItemOnGather;

		if (ApplicableGameplayItemDefinitionFragment->bUniqueCategory)
		{
            for (UNLGameplayItemInstance* Instance : GetAllItems())
			{
                const UNLGameplayItemFragment_ApplicableGameplayItemDefinition* OtherApplicableGameplayItemDefinitionFragment = Cast<UNLGameplayItemFragment_ApplicableGameplayItemDefinition>(NLGameplaySubsystem->FindItemDefinitionFragment(Instance->GetItemDef(), UNLGameplayItemFragment_ApplicableGameplayItemDefinition::StaticClass()));
                
				if (OtherApplicableGameplayItemDefinitionFragment && ApplicableGameplayItemDefinitionFragment->GameplayItemCategoryTags.HasAnyExact(OtherApplicableGameplayItemDefinitionFragment->GameplayItemCategoryTags))
				{
                    RemoveItemInstance(Instance);
				}
            }
		}
	}

	Result = GameplayItemList.AddEntry(ItemDef, StackCount);
		
	if (ApplicableGameplayItemDefinitionFragment && bApplyGameplayItemOnGather)
	{
        ApplicableGameplayItemManager->ApplyItem(ApplicableGameplayItemDefinitionFragment->ApplicableGameplayItemDefinition, Result);
	}

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
	{
		AddReplicatedSubObject(Result);
	}

	return Result;
}

void UNLGameplayItemManagerComponent::RemoveItemInstance(UNLGameplayItemInstance* ItemInstance)
{
    UNLApplicableGameplayItemManagerComponent* ApplicableGameplayItemManager = FindApplicableGameplayItemManager();
	UNLGameplaySubsystem* NLGameplaySubsystem = UWorld::GetSubsystem<UNLGameplaySubsystem>(GetOwner()->GetWorld());

	const UNLGameplayItemFragment_ApplicableGameplayItemDefinition* ApplicableGameplayItemDefinitionFragment = Cast<UNLGameplayItemFragment_ApplicableGameplayItemDefinition>(NLGameplaySubsystem->FindItemDefinitionFragment(ItemInstance->GetItemDef(), UNLGameplayItemFragment_ApplicableGameplayItemDefinition::StaticClass()));

	if (ApplicableGameplayItemDefinitionFragment)
	{
        ApplicableGameplayItemManager->UnapplyItemsByDefinition(ApplicableGameplayItemDefinitionFragment->ApplicableGameplayItemDefinition);
	}

	GameplayItemList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

TArray<UNLGameplayItemInstance*> UNLGameplayItemManagerComponent::GetAllItems() const
{
	return GameplayItemList.GetAllItems();
}

UNLGameplayItemInstance* UNLGameplayItemManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef) const
{
	for (const FNLGameplayItemEntry& Entry : GameplayItemList.Entries)
	{
		UNLGameplayItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 UNLGameplayItemManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FNLGameplayItemEntry& Entry : GameplayItemList.Entries)
	{
		UNLGameplayItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool UNLGameplayItemManagerComponent::ConsumeItemsByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	//@TODO: N squared right now as there's no acceleration structure
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (UNLGameplayItemInstance* Instance = UNLGameplayItemManagerComponent::FindFirstItemStackByDefinition(ItemDef))
		{
			GameplayItemList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

void UNLGameplayItemManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing UNLGameplayItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FNLGameplayItemEntry& Entry : GameplayItemList.Entries)
		{
			UNLGameplayItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool UNLGameplayItemManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FNLGameplayItemEntry& Entry : GameplayItemList.Entries)
	{
		UNLGameplayItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UNLGameplayItemManagerComponent::Server_AddItemDefinitions_Implementation(const TArray<FNLStackedGameplayItemDefition>& InStackedGameplayItemDefinitions)
{
	for (const FNLStackedGameplayItemDefition& Entry : InStackedGameplayItemDefinitions)
	{
        AddItemDefinition(Entry.ItemDef, Entry.StackCount);
	}
}

void UNLGameplayItemManagerComponent::Server_RemoveItemInstances_Implementation(const TArray<UNLGameplayItemInstance*>& InGameplayItemInstances)
{
    for (UNLGameplayItemInstance* Instance : InGameplayItemInstances)
	{
		RemoveItemInstance(Instance);
    }
}

UNLApplicableGameplayItemManagerComponent* UNLGameplayItemManagerComponent::FindApplicableGameplayItemManager() const
{
    if (AController* OwnerController = Cast<AController>(GetOwner())) {
        if (APawn* Pawn = OwnerController->GetPawn()) {
            return Pawn->FindComponentByClass<UNLApplicableGameplayItemManagerComponent>();
        }
    }

    return nullptr;
}


//////////////////////////////////////////////////////////////////////
//

// UCLASS(Abstract)
// class UNLGameplayItemFilter : public UObject
// {
// public:
// 	virtual bool PassesFilter(UNLGameplayItemInstance* Instance) const { return true; }
// };

// UCLASS()
// class UNLGameplayItemFilter_HasTag : public UNLGameplayItemFilter
// {
// public:
// 	virtual bool PassesFilter(UNLGameplayItemInstance* Instance) const { return true; }
// };


