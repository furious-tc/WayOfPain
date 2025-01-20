// Copyright 2025 Noblon GmbH. All Rights Reserved..

#include "System/NLGameInstance.h"
#include "CommonLocalPlayer.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameUIManagerSubsystem.h"
#include "NLGameplayTags.h"
#include "NLLogChannels.h"

UNLGameInstance::UNLGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super()
{
}

void UNLGameInstance::Init()
{
	Super::Init();

	// Register our custom init states
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(NLGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(NLGameplayTags::InitState_DataAvailable, false, NLGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(NLGameplayTags::InitState_DataInitialized, false, NLGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(NLGameplayTags::InitState_GameplayReady, false, NLGameplayTags::InitState_DataInitialized);
	}
}

void UNLGameInstance::Shutdown()
{
	Super::Shutdown();
}


int32 UNLGameInstance::AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId)
{
	int32 ReturnVal = Super::AddLocalPlayer(NewPlayer, UserId);
	if (ReturnVal != INDEX_NONE)
	{
		if (!PrimaryPlayer.IsValid())
		{
			UE_LOG(LogNL, Log, TEXT("AddLocalPlayer: Set %s to Primary Player"), *NewPlayer->GetName());
			PrimaryPlayer = NewPlayer;
		}

		GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerAdded(Cast<UCommonLocalPlayer>(NewPlayer));
	}

	return ReturnVal;
}

bool UNLGameInstance::RemoveLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	if (PrimaryPlayer == ExistingPlayer)
	{
		//TODO: do we want to fall back to another player?
		PrimaryPlayer.Reset();
		UE_LOG(LogNL, Log, TEXT("RemoveLocalPlayer: Unsetting Primary Player from %s"), *ExistingPlayer->GetName());
	}
	GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerDestroyed(Cast<UCommonLocalPlayer>(ExistingPlayer));

	return Super::RemoveLocalPlayer(ExistingPlayer);
}