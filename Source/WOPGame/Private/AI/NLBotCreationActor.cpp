// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "AI/NLBotCreationActor.h"
#include "System/NLGameMode.h"
#include "Player/NLPlayerState.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "Pawns/NLPawnData.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NLLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLBotCreationActor)

ANLBotCreationActor::ANLBotCreationActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent_Editor"));
	SphereComponent->InitSphereRadius(50.0f);
	RootComponent = SphereComponent;
#endif
}

void ANLBotCreationActor::BeginPlay()
{
	Super::BeginPlay();

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
}

#if WITH_SERVER_CODE
void ANLBotCreationActor::ServerCreateBots()
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	check(NavSys);

	for (int32 EntryIdx = 0; EntryIdx < BotCreationEntries.Num(); ++EntryIdx)
	{
		FNLBotCreationEntry& BotCreationEntry = BotCreationEntries[EntryIdx];

		for (int32 Idx = 0; Idx < BotCreationEntry.NumBotsToCreate; ++Idx)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnInfo.OverrideLevel = GetLevel();
			SpawnInfo.ObjectFlags |= RF_Transient;
			AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotCreationEntry.BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

			if (NewController != nullptr)
			{
                const UNLPawnData* NewBotPawnData = BotCreationEntry.BotPawnData;

				checkf(NewBotPawnData, TEXT("ANLBotCreationActor::ServerCreateBots(): Unable to find BotPawnData for [%s]!"), *BotCreationEntry.BotName);

				// For bots that use PlayerState
				if (ANLPlayerState* NLPS = NewController->GetPlayerState<ANLPlayerState>())
				{
                    NLPS->SetPawnData(NewBotPawnData);
				}

				ANLGameMode* GameMode = GetWorld()->GetAuthGameMode<ANLGameMode>();
				check(GameMode);

				FNavLocation NavLoc;
				NavSys->GetRandomReachablePointInRadius(GetActorLocation(), ZoneRadius, NavLoc);

				FRotator StartRotation(ForceInit);
				StartRotation.Yaw = (GetActorLocation() - NavLoc.Location).Rotation().Yaw;
				FVector StartLocation = NavLoc.Location;

				FTransform SpawnTransform = FTransform(StartRotation, StartLocation);

				APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(BotCreationEntry.BotPawnData->PawnClass, SpawnTransform, SpawnInfo);
				UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn); 
				PawnExtComp->SetPawnData(BotCreationEntry.BotPawnData);

				NewController->SetPawn(SpawnedPawn);

				if (!NewController->GetPawn())
				{
					GameMode->FailedToRestartPlayer(NewController);
				}
				else
				{
					GameMode->FinishRestartPlayer(NewController, StartRotation);
				}

				for (const UNLAbilitySet* AbilitySet : NewBotPawnData->AbilitySets) {
					if (AbilitySet) {
                        AbilitySet->GiveToAbilitySystem(PawnExtComp->GetNLAbilitySystemComponent(), &GrantedHandles);
					}
				}

				if (NewController->GetPawn() != nullptr)
				{
                    PawnExtComp->CheckDefaultInitialization();
				}

				SpawnedBotList.Add(NewController);
			}
		}
	}
}

#endif

#if WITH_EDITOR
void ANLBotCreationActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ANLBotCreationActor, ZoneRadius))
	{
		SphereComponent->SetSphereRadius(ZoneRadius);
	}
}
#endif
