// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "NLTeamStatics.generated.h"

class UNLTeamDisplayAsset;
class UObject;
class UTexture;
struct FFrame;

/** A subsystem for easy access to team information for team-based actors (e.g., pawns or player states) */
UCLASS()
class UNLTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Returns the team this object belongs to, or INDEX_NONE if it is not part of a team
	UFUNCTION(BlueprintCallable, Category=Teams, meta=(Keywords="GetTeamFromObject", DefaultToSelf="Agent", AdvancedDisplay="bLogIfNotSet"))
	static void FindTeamFromObject(const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, UNLTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet = false);

	UFUNCTION(BlueprintCallable, Category=Teams, meta=(WorldContext="WorldContextObject"))
	static UNLTeamDisplayAsset* GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId);

	UFUNCTION(BlueprintCallable, Category = Teams)
	static float GetTeamScalarWithFallback(UNLTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue);

	UFUNCTION(BlueprintCallable, Category = Teams)
	static FLinearColor GetTeamColorWithFallback(UNLTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue);

	UFUNCTION(BlueprintCallable, Category = Teams)
	static UTexture* GetTeamTextureWithFallback(UNLTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue);
};
