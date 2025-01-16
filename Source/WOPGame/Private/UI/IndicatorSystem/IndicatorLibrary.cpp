// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "UI/IndicatorSystem/IndicatorLibrary.h"

#include "UI/IndicatorSystem/NLIndicatorManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IndicatorLibrary)

class AController;

UIndicatorLibrary::UIndicatorLibrary()
{
}

UNLIndicatorManagerComponent* UIndicatorLibrary::GetIndicatorManagerComponent(AController* Controller)
{
	return UNLIndicatorManagerComponent::GetComponent(Controller);
}

