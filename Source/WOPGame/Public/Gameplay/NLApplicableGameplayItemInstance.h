// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Engine/World.h"
#include "Gameplay/NLGameplayItemInstance.h"
#include "Gameplay/NLApplicableGameplayItemDefinition.h"
#include "NativeGameplayTags.h"
#include "NLApplicableGameplayItemInstance.generated.h"

struct FNLApplicableGameplayItemActorToSpawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNLActorsSpawned);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAMEPLAYITEM_APPLICATION_STATE_MESSAGE);

USTRUCT(BlueprintType)
struct FNLAbilityGiveRemoveMessage {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APawn> Player = nullptr;

	UPROPERTY(BlueprintReadWrite)
    bool bApplied = false;

	UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UNLGameplayItemInstance> AssociatedGameplayItem;
};

/**
 * UNLApplicableGameplayItemInstance
 *
 * An applicable gameplay item spawned and applied to a pawn
 */
UCLASS(BlueprintType, Blueprintable)
class UNLApplicableGameplayItemInstance : public UObject
{
	GENERATED_BODY()
		
public:
	UNLApplicableGameplayItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	//~End of UObject interface

	UFUNCTION(BlueprintPure, Category=ApplicableGameplayItem)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category=ApplicableGameplayItem)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category=ApplicableGameplayItem, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category=ApplicableGameplayItem)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	UFUNCTION(BlueprintCallable, Category = GameplayItem)
	TArray<FNLApplicableGameplayItemActorToSpawn> GetApplicableItemActorsToSpawn() const;

	UFUNCTION(BlueprintCallable, Category = GameplayItem)
    TSubclassOf<UNLApplicableGameplayItemDefinition> GetApplicableItemDef() const
    {
        return ApplicableItemDef;
    }

	void SetApplicableItemDef(TSubclassOf<UNLApplicableGameplayItemDefinition> InDef);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = ApplicableGameplayItem)
    virtual void SpawnApplicableGameplayItemActors();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = ApplicableGameplayItem)
	virtual AActor* SpawnApplicableGameplayItemEntry(FNLApplicableGameplayItemActorToSpawn SpawnInfo);

	virtual void DestroyApplicableGameplayItemActors();

	virtual void OnApplied();
	virtual void OnUnapplied();

	UFUNCTION(BlueprintImplementableEvent, Category=ApplicableGameplayItem, meta=(DisplayName="OnApplied"))
    void K2_OnApplied();

	UFUNCTION(BlueprintImplementableEvent, Category=ApplicableGameplayItem, meta=(DisplayName="OnUnapplied"))
    void K2_OnUnapplied();

	void BroadcastApplicationStateMessage(bool bApplied);

	UPROPERTY(BlueprintAssignable, Category = ApplicableGameplayItem)
    FOnNLActorsSpawned OnActorsSpawned;

private:
	UFUNCTION()
	void OnRep_Instigator();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;

    UPROPERTY(Replicated)
    TSubclassOf<UNLApplicableGameplayItemDefinition> ApplicableItemDef;

	bool bApplicationStateBroadcasted = false;
};
