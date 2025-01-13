// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "NLVerbMessageHelpers.generated.h"

struct FGameplayCueParameters;
struct FNLVerbMessage;

class APlayerController;
class APlayerState;
class UObject;
struct FFrame;


UCLASS()
class WOPGAME_API UNLVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NL")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "NL")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "NL")
	static FGameplayCueParameters VerbMessageToCueParameters(const FNLVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "NL")
	static FNLVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};
