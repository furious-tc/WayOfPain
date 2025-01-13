// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "NLGameplayItemManagerComponent.generated.h"

class UNLGameplayItemDefinition;
class UNLGameplayItemInstance;
class UNLGameplayItemManagerComponent;
class UNLApplicableGameplayItemManagerComponent;
struct FNLGameplayItemList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/** A message when a GameplayItem is added */
USTRUCT(BlueprintType)
struct FNLGameplayItemChangeMessage
{
	GENERATED_BODY()

	//@TODO: Tag based names+owning actors for inventories instead of directly exposing the component?
	UPROPERTY(BlueprintReadOnly, Category=GameplayItem)
	TObjectPtr<UActorComponent> GameplayItemOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = GameplayItem)
	TObjectPtr<UNLGameplayItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=GameplayItem)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=GameplayItem)
	int32 Delta = 0;
};

/** A single entry for a GameplayItem */
USTRUCT(BlueprintType)
struct FNLGameplayItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FNLGameplayItemEntry()
	{}

	FString GetDebugString() const;

private:
	friend FNLGameplayItemList;
	friend UNLGameplayItemManagerComponent;

	UPROPERTY()
	TObjectPtr<UNLGameplayItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

/** List of GameplayItem items */
USTRUCT(BlueprintType)
struct FNLGameplayItemList : public FFastArraySerializer
{
	GENERATED_BODY()

	FNLGameplayItemList()
		: OwnerComponent(nullptr)
	{
	}

	FNLGameplayItemList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UNLGameplayItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNLGameplayItemEntry, FNLGameplayItemList>(Entries, DeltaParms, *this);
	}

	UNLGameplayItemInstance* AddEntry(TSubclassOf<UNLGameplayItemDefinition> ItemClass, int32 StackCount);

	void RemoveEntry(UNLGameplayItemInstance* Instance);

private:
	void BroadcastChangeMessage(FNLGameplayItemEntry& Entry, int32 OldCount, int32 NewCount);

private:
	friend UNLGameplayItemManagerComponent;

private:
	// Replicated list of items
	UPROPERTY()
	TArray<FNLGameplayItemEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

USTRUCT(BlueprintType)
struct FNLStackedGameplayItemDefition
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = GameplayItem)
    TSubclassOf<UNLGameplayItemDefinition> ItemDef;
        
	UPROPERTY(BlueprintReadWrite, Category = GameplayItem)
    int32 StackCount = 1;
};


template<>
struct TStructOpsTypeTraits<FNLGameplayItemList> : public TStructOpsTypeTraitsBase2<FNLGameplayItemList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Manages a GameplayItem
 */
UCLASS(BlueprintType, Meta = (BlueprintSpawnableComponent))
class WOPGAME_API UNLGameplayItemManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNLGameplayItemManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=GameplayItem)
	bool CanAddItemDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=GameplayItem)
	UNLGameplayItemInstance* AddItemDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=GameplayItem)
	void RemoveItemInstance(UNLGameplayItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=GameplayItem, BlueprintPure=false)
	TArray<UNLGameplayItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=GameplayItem, BlueprintPure)
	UNLGameplayItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef) const;

	int32 GetTotalItemCountByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef) const;
	bool ConsumeItemsByDefinition(TSubclassOf<UNLGameplayItemDefinition> ItemDef, int32 NumToConsume);

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface

	UFUNCTION(Server, Reliable, BlueprintCallable, Category=GameplayItem)
    void Server_AddItemDefinitions(const TArray<FNLStackedGameplayItemDefition>& InStackedGameplayItemDefinitions);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category=GameplayItem)
    void Server_RemoveItemInstances(const TArray<UNLGameplayItemInstance*>& InGameplayItemInstances);

private:

    UNLApplicableGameplayItemManagerComponent* FindApplicableGameplayItemManager() const;

private:
	UPROPERTY(Replicated)
	FNLGameplayItemList GameplayItemList;
};
