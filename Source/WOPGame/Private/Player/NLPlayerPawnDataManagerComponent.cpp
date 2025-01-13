// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Player/NLPlayerPawnDataManagerComponent.h"
#include "NLLogChannels.h"
#include "Pawns/NLPawnData.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "GameFramework/PlayerState.h"
#include "System/NLAssetManager.h"
#include "Player/NLPlayerState.h"
#include "Player/NLPlayerController.h"
#include "System/NLGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPlayerPawnDataManagerComponent)

UNLPlayerPawnDataManagerComponent::UNLPlayerPawnDataManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UNLPlayerPawnDataManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	ANLPlayerState* PS = Cast<ANLPlayerState>(GetOwner());
	check(PS);

	if (PS->GetPlayerController() && PS->GetPlayerController()->IsLocalController())
	{
		ANLPlayerController* NLPC = CastChecked<ANLPlayerController>(PS->GetPlayerController(), ECastCheckedType::NullChecked);
		NLPC->OnNLPlayerStateSet_RegisterAndCallOnce(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnNLPlayerStateSet));
	}
}

void UNLPlayerPawnDataManagerComponent::OnUnregister()
{
	Super::OnUnregister();
}

void UNLPlayerPawnDataManagerComponent::OnNLPlayerStateSet()
{
	ANLPlayerState* PS = Cast<ANLPlayerState>(GetOwner());
	check(PS);

	UNLAssetManager& AssetManager = UNLAssetManager::Get();
	const UNLPawnData* DesiredPawnData = AssetManager.GetAsset(DefaultPawnData);

	if (DesiredPawnData)
	{
		Server_ReplicateClientPawnData(DesiredPawnData);
	}
	else
	{
		UE_LOG(LogNL, Log, TEXT("PlayerPawnDataManagerComponent for NLPlayerState %s has no DefaultPawnData, relying on GameMode or other pawn spawning paths."), *PS->GetPlayerName());
	}
}

void UNLPlayerPawnDataManagerComponent::Server_ReplicateClientPawnData_Implementation(const UNLPawnData* InPawnData, const bool bKeepTransform)
{
	ANLPlayerState* PS = Cast<ANLPlayerState>(GetOwner());
	check(PS);

	PS->SetPawnData(InPawnData);

	ANLGameMode* GameMode = GetWorld()->GetAuthGameMode<ANLGameMode>();
	check(GameMode);

	AController* OwningController = PS->GetOwningController();

	FTransform SpawnTransform = GetSpawnTransform();

	if (IsValid(OwningController->GetPawn()))
	{
		if (bKeepTransform)
		{
			SpawnTransform = OwningController->GetPawn()->GetTransform();
		}

		OwningController->GetPawn()->Destroy();
	}

	if (GameMode->GetDefaultPawnClassForController(OwningController) != nullptr)
	{
		APawn* NewPawn = GameMode->SpawnDefaultPawnAtTransform(OwningController, SpawnTransform);

		if (IsValid(NewPawn))
		{
			OwningController->SetPawn(NewPawn);
		}
	}

	if (!IsValid(OwningController->GetPawn()))
	{
		GameMode->FailedToRestartPlayer(OwningController);
	}
	else
	{
		GameMode->FinishRestartPlayer(OwningController, SpawnTransform.Rotator());
	}
}

FTransform UNLPlayerPawnDataManagerComponent::GetSpawnTransform_Implementation() const
{
	ANLGameMode* GameMode = GetWorld()->GetAuthGameMode<ANLGameMode>();
	check(GameMode);

	ANLPlayerState* PS = Cast<ANLPlayerState>(GetOwner());
	check(PS);

	AActor* PlayerStartActor = GameMode->ChoosePlayerStart(PS->GetOwningController());

	return PlayerStartActor->GetTransform();
}
