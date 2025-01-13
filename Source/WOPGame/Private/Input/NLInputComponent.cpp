// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Input/NLInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLInputComponent)

class UNLInputConfig;

UNLInputComponent::UNLInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UNLInputComponent::AddInputMappings(const UNLInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
    check(InputConfig);
    check(InputSubsystem);

    // Here you can handle any custom logic to add something from your input config if required
}

void UNLInputComponent::RemoveInputMappings(const UNLInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
    check(InputConfig);
    check(InputSubsystem);

    // Here you can handle any custom logic to remove input mappings that you may have added above
}

void UNLInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
