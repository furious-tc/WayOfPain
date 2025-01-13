// Copyright 2025 Noblon GmbH. All Rights Reserved..

#include "System/NLGameInstance.h"
#include "Components/GameFrameworkComponentManager.h"
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
