// Copyright 2025 Noblon GmbH. All Rights Reserved..

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/GameState.h"
#include "NLGameState.generated.h"

struct FNLVerbMessage;

class APlayerState;
class UAbilitySystemComponent;
class UNLAbilitySystemComponent;

/**
 * ANLGameState
 *
 *	The base game state class used by Way of Pain.
 */
UCLASS(Config = Game)
class WOPGAME_API ANLGameState : public AGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANLGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	// Gets the ability system component used for game wide things
	UFUNCTION(BlueprintCallable, Category = "NL|GameState")
	UNLAbilitySystemComponent* GetNLAbilitySystemComponent() const { return AbilitySystemComponent; }

	// Gets the server's FPS, replicated to clients
	float GetServerFPS() const;

private:
	// The ability system component subobject for game-wide things (primarily gameplay cues)
	UPROPERTY(VisibleAnywhere, Category = "NL|GameState")
	TObjectPtr<UNLAbilitySystemComponent> AbilitySystemComponent;

protected:
	UPROPERTY(Replicated)
	float ServerFPS;

};
