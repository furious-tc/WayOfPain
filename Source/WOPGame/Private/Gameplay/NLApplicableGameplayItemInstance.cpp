// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Gameplay/NLApplicableGameplayItemInstance.h"
#include "Gameplay/NLApplicableGameplayItemDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLApplicableGameplayItemInstance)

UE_DEFINE_GAMEPLAY_TAG(TAG_GAMEPLAYITEM_APPLICATION_STATE_MESSAGE, "GameplayItem.Application.State.Message");

UNLApplicableGameplayItemInstance::UNLApplicableGameplayItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UNLApplicableGameplayItemInstance::GetWorld() const
{
	if (APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	else
	{
		return nullptr;
	}
}

void UNLApplicableGameplayItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
    DOREPLIFETIME(ThisClass, ApplicableItemDef);
}

APawn* UNLApplicableGameplayItemInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UNLApplicableGameplayItemInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;

	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}

	return Result;
}

void UNLApplicableGameplayItemInstance::SetApplicableItemDef(TSubclassOf<UNLApplicableGameplayItemDefinition> InDef)
{
    ApplicableItemDef = InDef;
}

TArray<FNLApplicableGameplayItemActorToSpawn> UNLApplicableGameplayItemInstance::GetApplicableItemActorsToSpawn() const
{
    const UNLApplicableGameplayItemDefinition* ApplicableGameplayItemCDO = GetDefault<UNLApplicableGameplayItemDefinition>(GetApplicableItemDef());
    return ApplicableGameplayItemCDO->ActorsToSpawn;
}

void UNLApplicableGameplayItemInstance::SpawnApplicableGameplayItemActors()
{
    check(GetPawn()->HasAuthority());

    const TArray<FNLApplicableGameplayItemActorToSpawn>& ActorsToSpawn = GetApplicableItemActorsToSpawn();

	for (const FNLApplicableGameplayItemActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		if (AActor* NewActor = SpawnApplicableGameplayItemEntry(SpawnInfo))
		{
            SpawnedActors.Add(NewActor);
		}
	}

	OnActorsSpawned.Broadcast();
}

AActor* UNLApplicableGameplayItemInstance::SpawnApplicableGameplayItemEntry(FNLApplicableGameplayItemActorToSpawn SpawnInfo)
{
    check(GetPawn()->HasAuthority());

	check(SpawnInfo.ActorToSpawn);

	AActor* NewActor = nullptr;

    if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

        NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
        NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/true);
        NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
        NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);
    }

	return NewActor;
}

void UNLApplicableGameplayItemInstance::DestroyApplicableGameplayItemActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UNLApplicableGameplayItemInstance::OnApplied()
{
    BroadcastApplicationStateMessage(true);

	K2_OnApplied();
}

void UNLApplicableGameplayItemInstance::OnUnapplied()
{
    BroadcastApplicationStateMessage(false);

	K2_OnUnapplied();
}

void UNLApplicableGameplayItemInstance::BroadcastApplicationStateMessage(bool bApplied)
{
	if (UNLGameplayItemInstance* GameplayItemInstance = Cast<UNLGameplayItemInstance>(GetInstigator()))
	{
        FNLAbilityGiveRemoveMessage Message;
        Message.Player = GetPawn();
        Message.bApplied = bApplied;
        Message.AssociatedGameplayItem = GameplayItemInstance;

        UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
        MessageSystem.BroadcastMessage(TAG_GAMEPLAYITEM_APPLICATION_STATE_MESSAGE, Message);

		bApplicationStateBroadcasted = true;
	}
}

void UNLApplicableGameplayItemInstance::OnRep_Instigator()
{
	if (!bApplicationStateBroadcasted)
	{
        BroadcastApplicationStateMessage(true);
	}
}

