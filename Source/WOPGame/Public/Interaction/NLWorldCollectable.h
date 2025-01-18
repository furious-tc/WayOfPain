// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Interaction/IPickupable.h"

#include "NLWorldCollectable.generated.h"

class UObject;
struct FInteractionQuery;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class ANLWorldCollectable : public AActor, public IInteractableTarget, public IPickupable
{
	GENERATED_BODY()

public:

	ANLWorldCollectable();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	virtual FGameplayItemPickup GetPickupGameplayItem() const override;

protected:
	UPROPERTY(EditAnywhere)
	FInteractionOption DefaultOption;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FInteractionOption> TaggedOptions;

	UPROPERTY(EditAnywhere)
	FGameplayItemPickup StaticGameplayItem;
};
