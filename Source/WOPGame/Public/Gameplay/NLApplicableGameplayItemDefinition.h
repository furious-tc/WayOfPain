// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"
#include "NLApplicableGameplayItemDefinition.generated.h"

class AActor;
class UNLAbilitySet;
class UNLApplicableGameplayItemInstance;

USTRUCT(BlueprintType)
struct FNLApplicableGameplayItemActorToSpawn {
	GENERATED_BODY()

	FNLApplicableGameplayItemActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ApplicableGameplayItem)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ApplicableGameplayItem)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ApplicableGameplayItem)
	FTransform AttachTransform;
};

/**
 * UNLApplicableGameplayItemDefinition
 *
 * Definition of an applicable gameplay item that can be applied to a pawn
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UNLApplicableGameplayItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UNLApplicableGameplayItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Class to spawn
	UPROPERTY(EditDefaultsOnly, Category=ApplicableGameplayItem)
	TSubclassOf<UNLApplicableGameplayItemInstance> InstanceType;

	// Gameplay ability sets to grant when this is applied
	UPROPERTY(EditDefaultsOnly, Category=ApplicableGameplayItem)
	TArray<TObjectPtr<const UNLAbilitySet>> AbilitySetsToGrant;

    // Whether the actor spawns manually by gameplay logic rather than upon application.
    UPROPERTY(EditDefaultsOnly, Category = ApplicableGameplayItem)
    bool bManagedSpawn = false;

	// Actors to spawn on the pawn when this is applied
	UPROPERTY(EditDefaultsOnly, Category=ApplicableGameplayItem)
	TArray<FNLApplicableGameplayItemActorToSpawn> ActorsToSpawn;

};
