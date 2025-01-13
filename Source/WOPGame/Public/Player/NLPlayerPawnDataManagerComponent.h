// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"
#include "NLPlayerPawnDataManagerComponent.generated.h"

class UNLPlayerPawnDataManagerComponent;

class UObject;
struct FFrame;

/**
 * UNLPlayerPawnDataManagerComponent
 *
 *	A player state component for player pawn data management.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class WOPGAME_API UNLPlayerPawnDataManagerComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UNLPlayerPawnDataManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Returns the player pawn data manager component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "NL|Player Pawn Data")
	static UNLPlayerPawnDataManagerComponent* FindPlayerPawnDataManagerComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UNLPlayerPawnDataManagerComponent>() : nullptr); }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NL|Player Pawn Data")
	TSoftObjectPtr<UNLPawnData const> DefaultPawnData;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "NL|Player Pawn Data")
	void Server_ReplicateClientPawnData(const UNLPawnData* InPawnData, const bool bKeepTransform = false);

protected:
	virtual void OnNLPlayerStateSet();

	// Returns the spawn transform for the new pawn.
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "NL|Player Pawn Data")
	FTransform GetSpawnTransform() const;

protected:
	virtual void OnUnregister() override;

};
