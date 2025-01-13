// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Abilities/NLAbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "NLApplicableGameplayItemManagerComponent.generated.h"

class UNLAbilitySystemComponent;
class UNLApplicableGameplayItemDefinition;
class UNLApplicableGameplayItemInstance;
class UNLApplicableGameplayItemManagerComponent;
struct FNLAppliedGameplayItemList;

/** A single piece of applied ApplicableGameplayItem */
USTRUCT(BlueprintType)
struct FNLAppliedGameplayItemEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    FNLAppliedGameplayItemEntry()
    {
    }

    FString GetDebugString() const;

private:
    friend FNLAppliedGameplayItemList;
    friend UNLApplicableGameplayItemManagerComponent;

    // The ApplicableGameplayItem class that got applied
    UPROPERTY()
    TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition;

    UPROPERTY()
    TObjectPtr<UNLApplicableGameplayItemInstance> Instance = nullptr;

    // Authority-only list of granted handles
    UPROPERTY(NotReplicated)
    FNLAbilitySet_GrantedHandles GrantedHandles;
};

/** List of applied ApplicableGameplayItem */
USTRUCT(BlueprintType)
struct FNLAppliedGameplayItemList : public FFastArraySerializer {
    GENERATED_BODY()

    FNLAppliedGameplayItemList()
        : OwnerComponent(nullptr)
    {
    }

    FNLAppliedGameplayItemList(UActorComponent* InOwnerComponent)
        : OwnerComponent(InOwnerComponent)
    {
    }

public:
    //~FFastArraySerializer contract
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
    //~End of FFastArraySerializer contract

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FNLAppliedGameplayItemEntry, FNLAppliedGameplayItemList>(Entries, DeltaParms, *this);
    }

    UNLApplicableGameplayItemInstance* AddEntry(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition);
    void RemoveEntry(UNLApplicableGameplayItemInstance* Instance);

private:
    UNLAbilitySystemComponent* GetAbilitySystemComponent() const;

    friend UNLApplicableGameplayItemManagerComponent;

private:
    // Replicated list of ApplicableGameplayItem entries
    UPROPERTY()
    TArray<FNLAppliedGameplayItemEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<UActorComponent> OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FNLAppliedGameplayItemList> : public TStructOpsTypeTraitsBase2<FNLAppliedGameplayItemList> {
    enum { WithNetDeltaSerializer = true };
};

/**ByDefinition
 * Manages ApplicableGameplayItem applied to a pawn
 */
UCLASS(BlueprintType, Const, Meta = (BlueprintSpawnableComponent))
class UNLApplicableGameplayItemManagerComponent : public UPawnComponent {
    GENERATED_BODY()

public:
    UNLApplicableGameplayItemManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    UNLApplicableGameplayItemInstance* ApplyItem(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition, UObject* Instigator = nullptr);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void UnapplyItem(UNLApplicableGameplayItemInstance* ItemInstance);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void UnapplyItemsByDefinition(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableGameplayItemDefinition);

    //~UObject interface
    virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
    //~End of UObject interface

    //~UActorComponent interface
    // virtual void EndPlay() override;
    virtual void InitializeComponent() override;
    virtual void UninitializeComponent() override;
    virtual void ReadyForReplication() override;
    //~End of UActorComponent interface

    /** Returns the first applied instance of a given type, or nullptr if none are found */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    UNLApplicableGameplayItemInstance* GetFirstInstanceOfType(TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType);

    /** Returns all applied instances of a given type, or an empty array if none are found */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<UNLApplicableGameplayItemInstance*> GetApplicableGameplayItemInstancesOfType(TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType) const;

     /** Returns all applied instances of a given definition, or an empty array if none are found */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<UNLApplicableGameplayItemInstance*> GetApplicableGameplayItemInstancesOfDefinition(TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableItemDefinition) const;

    template <typename T>
    T* GetFirstInstanceOfType()
    {
        return (T*)GetFirstInstanceOfType(T::StaticClass());
    }

private:
    UPROPERTY(Replicated)
    FNLAppliedGameplayItemList AppliedGameplayItemList;
};
