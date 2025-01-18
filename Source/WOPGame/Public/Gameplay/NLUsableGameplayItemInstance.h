// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Cosmetics/NLCosmeticAnimationTypes.h"
#include "Gameplay/NLApplicableGameplayItemInstance.h"
#include "GameFramework/InputDevicePropertyHandle.h"

#include "NLUsableGameplayItemInstance.generated.h"

class UAnimInstance;
class UObject;
struct FFrame;
struct FGameplayTagContainer;
class UInputDeviceProperty;

/**
 * UNLUsableGameplayItemInstance
 *
 * A usable gameplay item spawned and applied to a pawn
 */
UCLASS()
class WOPGAME_API UNLUsableGameplayItemInstance : public UNLApplicableGameplayItemInstance
{
	GENERATED_BODY()

public:
	UNLUsableGameplayItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UNLApplicableGameplayItemInstance interface
	virtual void OnApplied() override;
	virtual void OnUnapplied() override;
	//~End of UNLApplicableGameplayItemInstance interface

	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	// Returns how long it's been since the usable gameplay item was interacted with (used or applied)
	UFUNCTION(BlueprintPure)
	float GetTimeSinceLastInteractedWith() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FNLAnimLayerSelectionSet AppliedAnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FNLAnimLayerSelectionSet UnappliedAnimSet;

	/**
	 * Device properties that should be applied while this usable item is applied.
	 * These properties will be played in with the "Looping" flag enabled, so they will
	 * play continuously until this usable item is unapplied! 
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Input Devices")
	TArray<TObjectPtr<UInputDeviceProperty>> ApplicableDeviceProperties;
	
	// Choose the best layer from AppliedAnimSet or UnappliedAnimSet based on the specified gameplay tags
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Animation)
	TSubclassOf<UAnimInstance> PickBestAnimLayer(bool bApplied, const FGameplayTagContainer& CosmeticTags) const;

	/** Returns the owning Pawn's Platform User ID */
	UFUNCTION(BlueprintCallable)
	const FPlatformUserId GetOwningUserId() const;

	/** Callback for when the owning pawn of this usable gameplay item dies. Removes all spawned device properties. */
	UFUNCTION()
	void OnEliminationStarted(AActor* OwningActor);

	/**
	 * Apply the ApplicableDeviceProperties to the owning pawn of this usable gameplay item.
	 * Populate the DevicePropertyHandles so that they can be removed later. This will
	 * Play the device properties in Looping mode so that they will share the lifetime of the
	 * usable gameplay item being Applied.
	 */
	void ApplyDeviceProperties();

	/** Remove any device proeprties that were activated in ApplyDeviceProperties. */
	void RemoveDeviceProperties();

private:

	/** Set of device properties activated by this usable gameplay item. Populated by ApplyDeviceProperties */
	UPROPERTY(Transient)
	TSet<FInputDevicePropertyHandle> DevicePropertyHandles;

	double TimeLastApplied = 0.0;
	double TimeLastFired = 0.0;
};
