// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Interaction/IPickupable.h"

#include "GameFramework/Actor.h"
#include "Gameplay/NLGameplayItemManagerComponent.h"
#include "UObject/ScriptInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IPickupable)

class UActorComponent;

UPickupableStatics::UPickupableStatics()
	: Super(FObjectInitializer::Get())
{
}

TScriptInterface<IPickupable> UPickupableStatics::GetFirstPickupableFromActor(AActor* Actor)
{
	// If the actor is directly pickupable, return that.
	TScriptInterface<IPickupable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	// If the actor isn't pickupable, it might have a component that has a pickupable interface.
	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		// Get first pickupable, if the user needs more sophisticated pickup distinction, will need to be solved elsewhere.
		return TScriptInterface<IPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IPickupable>();
}

void UPickupableStatics::AddPickupToGameplayItem(UNLGameplayItemManagerComponent* GameplayItemComponent, TScriptInterface<IPickupable> Pickup)
{
	if (GameplayItemComponent && Pickup)
	{
		const FGameplayItemPickup& PickupGameplayItem = Pickup->GetPickupGameplayItem();

		for (const FPickupTemplate& Template : PickupGameplayItem.Templates)
		{
			GameplayItemComponent->AddItemDefinition(Template.ItemDef, Template.StackCount);
		}
	}
}
