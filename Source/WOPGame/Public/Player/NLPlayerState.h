// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Teams/NLTeamAgentInterface.h"
#include "GameFramework/PlayerState.h"
#include "System/GameplayTagStack.h"
#include "NLPlayerState.generated.h"

class UAbilitySystemComponent;
class UNLPawnData;

/**
 * ANLPlayerState
 *
 *	Base player state class used by Way of Pain.
 */
UCLASS(Config = Game)
class WOPGAME_API ANLPlayerState : public APlayerState, public IAbilitySystemInterface, public INLTeamAgentInterface
{
	GENERATED_BODY()
	
public:
    ANLPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "NL|Player State")
    UNLAbilitySystemComponent* GetNLAbilitySystemComponent() const { return AbilitySystemComponent; }
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

    UFUNCTION(BlueprintCallable, Category = "NL|Player State")
	void SetPawnData(const UNLPawnData* InPawnData);

    //~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	//~End of APlayerState interface

    //~INLTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnNLTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of INLTeamAgentInterface interface

    static const FName NAME_NLAbilityReady;

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(TeamID);
	}

	
	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;

	// Send a message to just this player
	// (use only for client notifications like accolades, quest toasts, etc... that can handle being occasionally lost)
	UFUNCTION(Client, Unreliable, BlueprintCallable, Category = "NL|PlayerState")
	void ClientBroadcastMessage(const FNLVerbMessage Message);

    FRotator GetReplicatedViewRotation() const;

    // Sets the replicated view rotation, only valid on the server
    void SetReplicatedViewRotation(const FRotator& NewRotation);
    
protected:
    UFUNCTION()
    void OnRep_PawnData();

protected:
    UPROPERTY(ReplicatedUsing = OnRep_PawnData)
    TObjectPtr<const UNLPawnData> PawnData;

private:

    // The Way of Pain ability system component used by player characters.
    UPROPERTY(VisibleAnywhere, Category = "NL|Player State")
    TObjectPtr<UNLAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class UNLHealthSet> HealthSet;

	UPROPERTY()
	TObjectPtr<const class UNLCombatSet> CombatSet;

	UPROPERTY()
	TObjectPtr<const class UNLMovementSet> MovementSet;

	UPROPERTY()
	FOnNLTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing = OnRep_TeamID)
	FGenericTeamId TeamID;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

    UPROPERTY(Replicated)
    FRotator ReplicatedViewRotation;

private:
	UFUNCTION()
	void OnRep_TeamID(FGenericTeamId OldTeamID);

};
