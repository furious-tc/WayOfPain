// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "Teams/NLTeamAgentInterface.h"
#include "Abilities/Attributes/NLHealthSet.h"
#include "Abilities/Attributes/NLCombatSet.h"
#include "NLCharacter.generated.h"

class ANLPlayerState;
class IRepChangedPropertyTracker;
class UAbilitySystemComponent;
class UInputComponent;
class UNLAbilitySystemComponent;
class UNLPawnExtensionComponent;
class UNLHealthComponent;
class UNLApplicableGameplayItemManagerComponent;
struct FGameplayTag;
struct FGameplayTagContainer;

/**
 * FNLReplicatedAcceleration: Compressed representation of acceleration
 */
USTRUCT()
struct FNLReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};

/**
 * ANLCharacter
 *
 *	The base character pawn class used by Way of Pain.
 *	Responsible for sending events to pawn components.
 *	New behavior should be added via pawn components when possible.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by Way of Pain."))
class WOPGAME_API ANLCharacter : public ACharacter, public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface, public INLTeamAgentInterface {
	GENERATED_BODY()

public:

	ANLCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "NL|Character")
	ANLPlayerState* GetNLPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "NL|Character")
	UNLAbilitySystemComponent* GetNLAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	//~End of AActor interface

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	//~INLTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnNLTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of INLTeamAgentInterface interface

public:

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetPrimaryAttachmentMesh"))
	USkeletalMeshComponent* GetPrimaryAttachmentMesh() const;

protected:

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void InitializeGameplayTags();

	virtual USkeletalMeshComponent* GetPrimaryAttachmentMesh_Implementation() const;

	// Begins the elimination sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnEliminationStarted(AActor* OwningActor);

	// Ends the elimination sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnEliminationFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToElimination();
	void UninitAndDestroy();

	// Called when the elimination sequence for the character has completed
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEliminationFinished"))
	void K2_OnEliminationFinished();

public:
	// The ability system component sub-object used by player characters.
    UPROPERTY(VisibleAnywhere, Category = "NL|Character")
    TObjectPtr<UNLAbilitySystemComponent> AbilitySystemComponent;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NL|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNLPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NL|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNLHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NL|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNLApplicableGameplayItemManagerComponent> ApplicableGameplayItemManagerComponent;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FNLReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_TeamID)
	FGenericTeamId TeamID;

	UPROPERTY()
	FOnNLTeamIndexChangedDelegate OnTeamChangedDelegate;

protected:
	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

private:
	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UFUNCTION()
	void OnRep_TeamID(FGenericTeamId OldTeamID);
};
