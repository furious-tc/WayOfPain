// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "NLHealthComponent.generated.h"

class UNLHealthComponent;

class UNLAbilitySystemComponent;
class UNLHealthSet;
class UObject;
struct FFrame;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNLHealth_EliminationEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FNLHealth_AttributeChanged, UNLHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/**
 * ENLEliminationState
 *
 *	Defines current state of Elimination.
 */
UENUM(BlueprintType)
enum class ENLEliminationState : uint8
{
	NotEliminated = 0,
	EliminationStarted,
	EliminationFinished
};


/**
 * UNLHealthComponent
 *
 *	An actor component used to handle anything related to health.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class WOPGAME_API UNLHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	UNLHealthComponent(const FObjectInitializer& ObjectInitializer);

	// Returns the health component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "NL|Health")
	static UNLHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UNLHealthComponent>() : nullptr); }

	// Initialize the component using an ability system component.
	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	void InitializeWithAbilitySystem(UNLAbilitySystemComponent* InASC);

	// Uninitialize the component, clearing any references to the ability system.
	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	void UninitializeFromAbilitySystem();

	// Returns the current health value.
	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	float GetHealth() const;

	// Returns the current maximum health value.
	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	float GetMaxHealth() const;

	// Returns the current health in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "NL|Health")
	ENLEliminationState GetEliminationState() const { return EliminationState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "NL|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsEliminatedOrBeingEliminated() const { return (EliminationState > ENLEliminationState::NotEliminated); }

	// Begins the Elimination sequence for the owner.
	virtual void StartElimination();

	// Ends the Elimination sequence for the owner.
	virtual void FinishElimination();

	// Applies enough damage to kill the owner.
	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

public:

	// Delegate fired when the health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
	FNLHealth_AttributeChanged OnHealthChanged;

	// Delegate fired when the max health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
	FNLHealth_AttributeChanged OnMaxHealthChanged;

	// Delegate fired when the Elimination sequence has started.
	UPROPERTY(BlueprintAssignable)
	FNLHealth_EliminationEvent OnEliminationStarted;

	// Delegate fired when the Elimination sequence has finished.
	UPROPERTY(BlueprintAssignable)
	FNLHealth_EliminationEvent OnEliminationFinished;

protected:

	virtual void OnUnregister() override;

	void ClearGameplayTags();

	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
	virtual void OnRep_EliminationState(ENLEliminationState OldEliminationState);

protected:

	// Ability system used by this component.
	UPROPERTY()
	TObjectPtr<UNLAbilitySystemComponent> AbilitySystemComponent;

	// Health set used by this component.
	UPROPERTY()
	TObjectPtr<const UNLHealthSet> HealthSet;

	// Replicated state used to handle BeingEliminated.
	UPROPERTY(ReplicatedUsing = OnRep_EliminationState)
	ENLEliminationState EliminationState;
};
