// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Interaction/NLWorldCollectable.h"
#include "Interaction/InteractionQuery.h"

#include "Async/TaskGraphInterfaces.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLWorldCollectable)

struct FInteractionQuery;

ANLWorldCollectable::ANLWorldCollectable()
{
}

void ANLWorldCollectable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	const FInteractionOption* TaggedOption = TaggedOptions.Find(InteractQuery.OptionalTag);
	
	InteractionBuilder.AddInteractionOption(TaggedOption ? *TaggedOption : DefaultOption);
}

FGameplayItemPickup ANLWorldCollectable::GetPickupGameplayItem() const
{
	return StaticGameplayItem;
}
