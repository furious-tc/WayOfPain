// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "GameplayEffectTypes.h"
#include "Components/PawnComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "NLPawnEffectManagerComponent.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UGameFrameworkComponentManager;
class UNLAbilitySystemComponent;
struct FGameplayTag;

/** A message for PawnEffect stack changed. */
USTRUCT(BlueprintType)
struct FNLPawnEffectStackChangeMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=PawnEffect)
    TObjectPtr<APawn> Player = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = PawnEffect)
    FActiveGameplayEffectHandle EffectHandle;

	UPROPERTY(BlueprintReadOnly, Category=PawnEffect)
    int32 NewStackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = PawnEffect)
    int32 PreviousStackCount = 0;
	
	UPROPERTY(BlueprintReadOnly, Category=PawnEffect)
	int32 Delta = 0;
};

/**
 * Component that propagates pawn's Ability System Component effects upon relevancy.
 * This depends on a PawnExtensionComponent to coordinate initialization.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class WOPGAME_API UNLPawnEffectManagerComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UNLPawnEffectManagerComponent(const FObjectInitializer& ObjectInitializer);

	/** Returns the PawnEffect component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "NL|PawnEffect")
	static UNLPawnEffectManagerComponent* FindPawnEffectComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UNLPawnEffectManagerComponent>() : nullptr); }

	/** The name of this component-implemented feature */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	// Gets active pawn gameplay effect handles.
	UFUNCTION(BlueprintCallable, Category = "NL|PawnEffect")
	TArray<FActiveGameplayEffectHandle> GetPawnGameplayEffectHandles() const;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnAbilitySystemInitialized();
    virtual void OnAbilitySystemUninitialized();

	virtual void OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
    virtual void OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved);
    virtual void OnGameplayEffectStackChange(FActiveGameplayEffectHandle EffectHandle, int32 NewStackCount, int32 PreviousStackCount);

	virtual void BroadcastChangeMessage(FActiveGameplayEffectHandle EffectHandle, int32 NewStackCount, int32 PreviousStackCount);

protected:
    UNLAbilitySystemComponent* NLASC;

};
