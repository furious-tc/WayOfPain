// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "IndicatorLibrary.generated.h"

class AController;
class UNLIndicatorManagerComponent;
class UObject;
struct FFrame;

UCLASS()
class WOPGAME_API UIndicatorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UIndicatorLibrary();
	
	/**  */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	static UNLIndicatorManagerComponent* GetIndicatorManagerComponent(AController* Controller);
};
