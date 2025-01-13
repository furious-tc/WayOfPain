// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#include "System/NLGameMode.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "NLLogChannels.h"
#include "Misc/CommandLine.h"
#include "System/NLAssetManager.h"
#include "System/NLGameState.h"
#include "System/NLGameSession.h"
#include "Player/NLPlayerState.h"
#include "Pawns/NLCharacter.h"
#include "UI/NLHUD.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "Pawns/NLPawnData.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameMapsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameMode)

ANLGameMode::ANLGameMode(const FObjectInitializer& ObjectInitializer)
    : Super()
{
    GameStateClass = ANLGameState::StaticClass();
    GameSessionClass = ANLGameSession::StaticClass();
    PlayerStateClass = ANLPlayerState::StaticClass();
    DefaultPawnClass = ANLCharacter::StaticClass();
    HUDClass = ANLHUD::StaticClass();
}

const UNLPawnData* ANLGameMode::GetPawnDataForController(const AController* InController) const
{
    // See if pawn data is already set on the player state
    if (InController != nullptr) {
        if (const ANLPlayerState* NLPS = InController->GetPlayerState<ANLPlayerState>()) {
            if (const UNLPawnData* PawnData = NLPS->GetPawnData<UNLPawnData>()) {
                return PawnData;
            }
        }
    }

    // Pawn data not yet set, so there is no pawn data to be had
    return nullptr;
}

void ANLGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
}

UClass* ANLGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    const UNLPawnData* PawnData = GetPawnDataForController(InController);

    if (PawnData) {
        if (PawnData->PawnClass) {
            return PawnData->PawnClass;
        }
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ANLGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = GetInstigator();
    SpawnInfo.ObjectFlags |= RF_Transient; // Never save the default player pawns into a map.
    SpawnInfo.bDeferConstruction = true;

    if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer)) 
    {
        if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo)) 
        {
            if (UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn)) 
            {
                if (const UNLPawnData* PawnData = GetPawnDataForController(NewPlayer)) 
                {
                    PawnExtComp->SetPawnData(PawnData);
                } 
                else 
                {
                    UE_LOG(LogNL, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."), *GetNameSafe(SpawnedPawn));
                }
            }

            SpawnedPawn->FinishSpawning(SpawnTransform);

            return SpawnedPawn;
        } 
        else 
        {
            UE_LOG(LogNL, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
        }
    } 
    else 
    {
        UE_LOG(LogNL, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
    }

    return nullptr;
}

bool ANLGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
    // We never want to use the start spot, always use the spawn management component.
    return false;
}

void ANLGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{

}

AActor* ANLGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    return Super::ChoosePlayerStart_Implementation(Player);
}

void ANLGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
    Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool ANLGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
    return ControllerCanRestart(Player);
}

bool ANLGameMode::ControllerCanRestart(AController* Controller)
{
    if (APlayerController* PC = Cast<APlayerController>(Controller)) {
        if (!Super::PlayerCanRestart_Implementation(PC)) {
            return false;
        }
    } else {
        // Bot version of Super::PlayerCanRestart_Implementation
        if ((Controller == nullptr) || Controller->IsPendingKillPending()) {
            return false;
        }
    }

    return true;
}

void ANLGameMode::InitGameState()
{
    Super::InitGameState();
}

void ANLGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
    Super::GenericPlayerInitialization(NewPlayer);

    OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void ANLGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bFNLeReset)
{
    if (bFNLeReset && (Controller != nullptr)) {
        Controller->Reset();
    }

    if (APlayerController* PC = Cast<APlayerController>(Controller)) {
        GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
    }
}

bool ANLGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
    // Do nothing, we'll wait until PostLogin when we try to spawn the player for real.
    // Doing anything right now is no good, systems like team assignment haven't even occurred yet.
    return true;
}

void ANLGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
    Super::FailedToRestartPlayer(NewPlayer);

    // If we tried to spawn a pawn and it failed, lets try again *note* check if there's actually a pawn class
    // before we try this forever.
    if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer)) {
        if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer)) {
            // If it's a player don't loop forever, maybe something changed and they can no longer restart if so stop trying.
            if (PlayerCanRestart(NewPC)) {
                RequestPlayerRestartNextFrame(NewPlayer, false);
            } else {
                UE_LOG(LogNL, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
            }
        } else {
            RequestPlayerRestartNextFrame(NewPlayer, false);
        }
    } else {
        UE_LOG(LogNL, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
    }
}
