// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/NLWorldCollectable.h"
#include "Interaction/InteractionOption.h"
#include "NLGameplayItemInteractPickup.generated.h"

namespace EEndPlayReason { enum Type : int; }

class APawn;
class UCapsuleComponent;
class UNLGameplayItemDefinition;
class UNLGameplayItemPickupDefinition;
class UObject;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FFrame;
struct FGameplayTag;
struct FHitResult;

UCLASS(Blueprintable,BlueprintType)
class WOPGAME_API ANLGameplayItemInteractPickup : public ANLWorldCollectable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANLGameplayItemInteractPickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnConstruction(const FTransform& Transform) override;

protected:
	//Data asset used to configure a GameplayItem pickup
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	TObjectPtr<UNLGameplayItemPickupDefinition> GameplayItemPickupDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_GameplayItemAvailability, Category = "NL|GameplayItemPickup")
	bool bIsGameplayItemAvailable;

	//The amount of time between GameplayItem pickup and GameplayItem spawning in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	float CoolDownTime;

	//Delay between when the GameplayItem is made available and when we check for a pawn standing in the spawner. Used to give the bIsGameplayItemAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	float CheckExistingOverlapDelay;

	//Used to drive GameplayItem respawn time indicators 0-1
	UPROPERTY(BlueprintReadOnly, Transient, Category = "NL|GameplayItemPickup")
	float CoolDownPercentage;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	bool bCanRespawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(BlueprintReadOnly, Category = "NL|GameplayItemPickup")
	TObjectPtr<UStaticMeshComponent> GameplayItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "NL|GameplayItemPickup")
	float GameplayItemMeshRotationSpeed;

	FTimerHandle CoolDownTimerHandle;

	FTimerHandle CheckOverlapsDelayTimerHandle;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	//Check for pawns standing on pad when the GameplayItem is spawned. 
	void CheckForExistingOverlaps();

	UFUNCTION(BlueprintNativeEvent)
	void AttemptPickUpGameplayItem(APawn* Pawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "NL|GameplayItemPickup")
	bool GiveGameplayItem(TSubclassOf<UNLGameplayItemDefinition> GameplayItemClass, APawn* ReceivingPawn);

	void StartCoolDown();

	UFUNCTION(BlueprintCallable, Category = "NL|GameplayItemPickup")
	void ResetCoolDown();

	UFUNCTION()
	void OnCoolDownTimerComplete();

	void SetGameplayItemPickupVisibility(bool bShouldBeVisible);

	UFUNCTION(BlueprintNativeEvent, Category = "NL|GameplayItemPickup")
	void PlayPickupEffects();

	UFUNCTION(BlueprintNativeEvent, Category = "NL|GameplayItemPickup")
	void PlayRespawnEffects();

	UFUNCTION()
	void OnRep_GameplayItemAvailability();

	/** Searches an item definition type for a matching stat and returns the value, or 0 if not found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NL|GameplayItemPickup")
	static int32 GetDefaultStatFromItemDef(const TSubclassOf<UNLGameplayItemDefinition> GameplayItemClass, FGameplayTag StatTag);
};
