// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Gameplay/NLGameplayItemDefinition.h"
#include "Styling/SlateBrush.h"
#include "NLGameplayItemFragment_UIAppearance.generated.h"

class UObject;

UCLASS()
class UNLGameplayItemFragment_UIAppearance : public UNLGameplayItemFragment {

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FText DisplayNameWhenApplied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush Brush;

};
