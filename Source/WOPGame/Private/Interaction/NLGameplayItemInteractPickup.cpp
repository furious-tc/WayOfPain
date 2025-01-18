// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Interaction/NLGameplayItemInteractPickup.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gameplay/NLGameplayItemPickupDefinition.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/NLGameplayItemFragment_SetStats.h"
#include "Kismet/GameplayStatics.h"
#include "NLLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameplayItemInteractPickup)

class FLifetimeProperty;
class UNiagaraSystem;
class USoundBase;
struct FHitResult;

// Sets default values
ANLGameplayItemInteractPickup::ANLGameplayItemInteractPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ANLGameplayItemInteractPickup::OnOverlapBegin);

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);

	GameplayItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GameplayItemMesh"));
	GameplayItemMesh->SetupAttachment(RootComponent);

	GameplayItemMeshRotationSpeed = 40.0f;
	CoolDownTime = 30.0f;
	CheckExistingOverlapDelay = 0.25f;
	bIsGameplayItemAvailable = true;
	bReplicates = true;
}

// Called when the game starts or when spawned
void ANLGameplayItemInteractPickup::BeginPlay()
{
	Super::BeginPlay();

	if (GameplayItemPickupDefinition && GameplayItemPickupDefinition->GameplayItemDefinition)
	{
		CoolDownTime = GameplayItemPickupDefinition->SpawnCoolDownSeconds;
	}
	else if (const UWorld* World = GetWorld())
	{
		if (!World->IsPlayingReplay())
		{
			UE_LOG(LogNL, Error, TEXT("'%s' does not have a valid GameplayItem definition! Make sure to set this data on the instance!"), *GetNameSafe(this));	
		}
	}
}

void ANLGameplayItemInteractPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
		World->GetTimerManager().ClearTimer(CheckOverlapsDelayTimerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ANLGameplayItemInteractPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update the CoolDownPercentage property to drive respawn time indicators
	UWorld* World = GetWorld();
	if (World->GetTimerManager().IsTimerActive(CoolDownTimerHandle))
	{
		CoolDownPercentage = 1.0f - World->GetTimerManager().GetTimerRemaining(CoolDownTimerHandle)/CoolDownTime;
	}

	if (GameplayItemMeshRotationSpeed > 0.f)
	{
		GameplayItemMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * GameplayItemMeshRotationSpeed, 0.0f));
	}
}

void ANLGameplayItemInteractPickup::OnConstruction(const FTransform& Transform)
{
	if (GameplayItemPickupDefinition != nullptr && GameplayItemPickupDefinition->DisplayMesh != nullptr)
	{
		GameplayItemMesh->SetStaticMesh(GameplayItemPickupDefinition->DisplayMesh);
	}	
}

void ANLGameplayItemInteractPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (GetLocalRole() == ROLE_Authority && bIsGameplayItemAvailable && OverlappingPawn != nullptr)
	{
		AttemptPickUpGameplayItem(OverlappingPawn);
	}
}

void ANLGameplayItemInteractPickup::CheckForExistingOverlaps()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		AttemptPickUpGameplayItem(Cast<APawn>(OverlappingActor));
	}
}

void ANLGameplayItemInteractPickup::AttemptPickUpGameplayItem_Implementation(APawn* Pawn)
{
	if (GetLocalRole() == ROLE_Authority && bIsGameplayItemAvailable && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		TSubclassOf<UNLGameplayItemDefinition> GameplayItemDefinition = GameplayItemPickupDefinition ? GameplayItemPickupDefinition->GameplayItemDefinition : nullptr;
		if (GameplayItemDefinition != nullptr)
		{
			//Attempt to grant the GameplayItem
			if (GiveGameplayItem(GameplayItemDefinition, Pawn))
			{
				//GameplayItem picked up by pawn
				bIsGameplayItemAvailable = false;
				SetGameplayItemPickupVisibility(false);
				PlayPickupEffects();
				StartCoolDown();
			}
		}		
	}
}

void ANLGameplayItemInteractPickup::StartCoolDown()
{
	if (!bCanRespawn) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoolDownTimerHandle, this, &ANLGameplayItemInteractPickup::OnCoolDownTimerComplete, CoolDownTime);
	}
}

void ANLGameplayItemInteractPickup::ResetCoolDown()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		bIsGameplayItemAvailable = true;
		PlayRespawnEffects();
		SetGameplayItemPickupVisibility(true);

		if (World)
		{
			World->GetTimerManager().SetTimer(CheckOverlapsDelayTimerHandle, this, &ANLGameplayItemInteractPickup::CheckForExistingOverlaps, CheckExistingOverlapDelay);
		}
	}

	CoolDownPercentage = 0.0f;
}

void ANLGameplayItemInteractPickup::OnCoolDownTimerComplete()
{
	ResetCoolDown();
}

void ANLGameplayItemInteractPickup::SetGameplayItemPickupVisibility(bool bShouldBeVisible)
{
	GameplayItemMesh->SetVisibility(bShouldBeVisible, true);
}

void ANLGameplayItemInteractPickup::PlayPickupEffects_Implementation()
{
	if (GameplayItemPickupDefinition != nullptr)
	{
		USoundBase* PickupSound = GameplayItemPickupDefinition->PickedUpSound;
		if (PickupSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}

		UNiagaraSystem* PickupEffect = GameplayItemPickupDefinition->PickedUpEffect;
		if (PickupEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, GameplayItemMesh->GetComponentLocation());
		}
	}
}

void ANLGameplayItemInteractPickup::PlayRespawnEffects_Implementation()
{
	if (GameplayItemPickupDefinition != nullptr)
	{
		USoundBase* RespawnSound = GameplayItemPickupDefinition->RespawnedSound;
		if (RespawnSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}

		UNiagaraSystem* RespawnEffect = GameplayItemPickupDefinition->RespawnedEffect;
		if (RespawnEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RespawnEffect, GameplayItemMesh->GetComponentLocation());
		}
	}
}

void ANLGameplayItemInteractPickup::OnRep_GameplayItemAvailability()
{
	if (bIsGameplayItemAvailable)
	{
		PlayRespawnEffects();
		SetGameplayItemPickupVisibility(true);
	}
	else
	{
		SetGameplayItemPickupVisibility(false);
		StartCoolDown();
		PlayPickupEffects();
	}	
}

void ANLGameplayItemInteractPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANLGameplayItemInteractPickup, bIsGameplayItemAvailable);
}

int32 ANLGameplayItemInteractPickup::GetDefaultStatFromItemDef(const TSubclassOf<UNLGameplayItemDefinition> GameplayItemClass, FGameplayTag StatTag)
{
	if (GameplayItemClass != nullptr)
	{
		if (UNLGameplayItemDefinition* GameplayItemCDO = GameplayItemClass->GetDefaultObject<UNLGameplayItemDefinition>())
		{
			if (const UNLGameplayItemFragment_SetStats* ItemStatsFragment = Cast<UNLGameplayItemFragment_SetStats>( GameplayItemCDO->FindFragmentByClass(UNLGameplayItemFragment_SetStats::StaticClass()) ))
			{
				return ItemStatsFragment->GetItemStatByTag(StatTag);
			}
		}
	}

	return 0;
}
