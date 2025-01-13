// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "NLAimSensitivityData.generated.h"

enum class ENLGamepadSensitivity : uint8;

class UObject;

/** Defines a set of gamepad sensitivity to a float value. */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "NL Aim Sensitivity Data", ShortTooltip = "Data asset used to define a map of Gamepad Sensitivty to a float value."))
class WOPGAME_API UNLAimSensitivityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UNLAimSensitivityData(const FObjectInitializer& ObjectInitializer);
	
	const float SensitivtyEnumToFloat(const ENLGamepadSensitivity InSensitivity) const;
	
protected:
	/** Map of SensitivityMap settings to their corresponding float */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ENLGamepadSensitivity, float> SensitivityMap;
};
