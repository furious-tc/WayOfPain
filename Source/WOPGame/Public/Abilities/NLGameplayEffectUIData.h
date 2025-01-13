// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameplayEffectUIData.h"
#include "NLGameplayEffectUIData.generated.h"

struct FSlateBrush;

/**
 * UI data for NL Gameplay Effects.
 */
UCLASS(DisplayName="UI Data (NL)")
class WOPGAME_API UNLGameplayEffectUIData : public UGameplayEffectUIData
{
    GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Data, meta = (MultiLine = "true"))
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Data, meta = (MultiLine = "true"))
	FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Data, meta = (MultiLine = "true"))
    FSlateBrush Brush;
};
