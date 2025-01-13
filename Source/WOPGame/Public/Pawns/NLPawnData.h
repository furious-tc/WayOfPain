// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "NLPawnData.generated.h"

class APawn;
class UNLAbilitySet;
class UNLAbilityTagRelationshipMapping;
class UNLInputConfig;
class UObject;

/**
 * UNLPawnData
 *
 *	Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Way of Pain Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class WOPGAME_API UNLPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UNLPawnData(const FObjectInitializer& ObjectInitializer);

public:

	// Class to instantiate for this pawn (should usually derive from Way of Pain's Character classe(s)).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|Abilities")
	TArray<TObjectPtr<UNLAbilitySet>> AbilitySets;

	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|Abilities")
	TObjectPtr<UNLAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|Input")
	TObjectPtr<UNLInputConfig> InputConfig;
};
