// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
#include "NLCharacterMovementComponent.generated.h"

class UNLAbilitySystemComponent;
class UNLMovementSet;
class UObject;
struct FFrame;
struct FGameplayEffectSpec;

WOPGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);

/**
 * UNLCharacterMovementComponent
 *
 *	The base character movement component class used by this project.
 */
UCLASS(Config = Game)
class WOPGAME_API UNLCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	class FSavedMove_NL : public FSavedMove_Character
	{
	public:
		enum CompressedFlags
		{
			FLAG_SetPredictedMovementSpeed			= 0x10,
			FLAG_Custom_1			= 0x20,
			FLAG_Custom_2		= 0x40,
			FLAG_Custom_3		= 0x80,
		};
		
		// Flags
		uint8 Saved_bPredictedMovementSetChange:1;

		FSavedMove_NL();

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_NL : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_NL(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

public:

	UNLCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	/** Returns the NL character movement component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "NL|CharacterMovement")
	static UNLCharacterMovementComponent* FindCharacterMovementComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UNLCharacterMovementComponent>() : nullptr); }

	// Initialize the component using an ability system component.
	virtual void InitializeAbilitySystem(UNLAbilitySystemComponent* InASC);

	// Uninitialize the component, clearing any references to the ability system.
	virtual void UninitializeAbilitySystem();

	virtual void SimulateMovement(float DeltaTime) override;

	void SetReplicatedAcceleration(const FVector& InAcceleration);

	//~UMovementComponent interface
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;
	//~End of UMovementComponent interface

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:

	virtual void InitializeComponent() override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	// 	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void HandleMovementSpeedChanged_GAS(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue);

protected:

	// Flags
	bool Request_bPredictedMovementSetChange;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;

	UPROPERTY(Transient)
	float InitialMaxWalkSpeed;

	// Ability system used by this component.
	UPROPERTY(Transient)
	TObjectPtr<UNLAbilitySystemComponent> AbilitySystemComponent;

	// MovementSpeed set used by this component.
	UPROPERTY(Transient)
	TObjectPtr<const UNLMovementSet> MovementSet;

};
