// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "Teams/NLTeamAgentInterface.h"
#include "NLPlayerController.generated.h"

class ANLHUD;
class ANLPlayerState;
class APawn;
class APlayerState;
class IInputInterface;
class UNLSettingsShared;
class UNLAbilitySystemComponent;
class UNLGameplayItemManagerComponent;

/**
 * ANLPlayerController
 *
 *	The base player controller class used by Way of Pain.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by Way of Pain."))
class WOPGAME_API ANLPlayerController : public APlayerController, public INLTeamAgentInterface
{
	GENERATED_BODY()

public:

	ANLPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "NL|PlayerController")
	ANLPlayerState* GetNLPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "NL|PlayerController")
	UNLAbilitySystemComponent* GetNLAbilitySystemComponent() const;
	
	UFUNCTION(BlueprintCallable, Category = "NL|PlayerController")
	ANLHUD* GetNLHUD() const;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	/** Register with the OnNLPlayerStateSet delegate and broadcast if our NLPlayerState has been set */
	void OnNLPlayerStateSet_RegisterAndCallOnce(FSimpleMulticastDelegate::FDelegate Delegate);

	virtual void OnRep_PlayerState() override;

	//~APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetPlayer(UPlayer* InPlayer) override;
	virtual void UpdateForceFeedback(IInputInterface* InputInterface, const int32 ControllerId) override;
	virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& OutHiddenComponents) override;
	//~End of APlayerController interface

	//~INLTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnNLTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of INLTeamAgentInterface interface

	UFUNCTION(BlueprintNativeEvent, Category = "NL|PlayerController")
	FVector2D GetMovementInputScaleValue(const FVector2D InValue) const;
	virtual FVector2D GetMovementInputScaleValue_Implementation(const FVector2D InValue) const;

	UPROPERTY()
	TObjectPtr<UNLGameplayItemManagerComponent> GameplayItemManagerComponent;

private:
	UPROPERTY()
	FOnNLTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:

	//~APlayerController interface

	//~End of APlayerController interface

	void OnSettingsChanged(UNLSettingsShared* Settings);

	bool bHideViewTargetPawnNextFrame = false;

protected:
	/** Delegate fired when our NLPlayerState has been set */
	FSimpleMulticastDelegate OnNLPlayerStateSet;

};

