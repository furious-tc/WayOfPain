// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Pawns/NLCharacter.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Pawns/NLCharacterMovementComponent.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "Pawns/NLHealthComponent.h"
#include "Gameplay/NLApplicableGameplayItemManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NLGameplayTags.h"
#include "NLLogChannels.h"
#include "Player/NLPlayerState.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLCharacter)

class IRepChangedPropertyTracker;
class UInputComponent;

ANLCharacter::ANLCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UNLCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetNetCullDistanceSquared(900000000.0f);

	PawnExtComponent = CreateDefaultSubobject<UNLPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	
	HealthComponent = CreateDefaultSubobject<UNLHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnEliminationStarted.AddDynamic(this, &ThisClass::OnEliminationStarted);
	HealthComponent->OnEliminationFinished.AddDynamic(this, &ThisClass::OnEliminationFinished);

	ApplicableGameplayItemManagerComponent = CreateDefaultSubobject<UNLApplicableGameplayItemManagerComponent>(TEXT("ApplicableGameplayItemManagerComponent"));
}

void ANLCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ANLCharacter::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

}

void ANLCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UWorld* World = GetWorld();
}

void ANLCharacter::Reset()
{
	K2_OnReset();

	UninitAndDestroy();
}

void ANLCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
	DOREPLIFETIME(ThisClass, TeamID)
}

void ANLCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
}

void ANLCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (Controller != nullptr))
	{
		if (INLTeamAgentInterface* ControllerWithTeam = Cast<INLTeamAgentInterface>(Controller))
		{
			TeamID = ControllerWithTeam->GetGenericTeamId();
			ConditionalBroadcastTeamChanged(this, OldTeamId, TeamID);
		}
	}
}

ANLPlayerState* ANLCharacter::GetNLPlayerState() const
{
	return CastChecked<ANLPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UNLAbilitySystemComponent* ANLCharacter::GetNLAbilitySystemComponent() const
{
	return Cast<UNLAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* ANLCharacter::GetAbilitySystemComponent() const
{
	if (PawnExtComponent == nullptr)
	{
		return nullptr;
	}

	return PawnExtComponent->GetNLAbilitySystemComponent();
}

void ANLCharacter::OnAbilitySystemInitialized()
{
	UNLAbilitySystemComponent* NLASC = PawnExtComponent->GetNLAbilitySystemComponent();
	check(NLASC);

	HealthComponent->InitializeWithAbilitySystem(NLASC);

	UNLCharacterMovementComponent* NLMoveComp = CastChecked<UNLCharacterMovementComponent>(GetCharacterMovement());
	NLMoveComp->InitializeAbilitySystem(NLASC);

	InitializeGameplayTags();
}

void ANLCharacter::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();

	UNLCharacterMovementComponent* NLMoveComp = CastChecked<UNLCharacterMovementComponent>(GetCharacterMovement());
	NLMoveComp->UninitializeAbilitySystem();
}

void ANLCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = TeamID;

	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();

	// Grab the current team ID and listen for future changes
	if (INLTeamAgentInterface* ControllerAsTeamProvider = Cast<INLTeamAgentInterface>(NewController))
	{
		TeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
}

void ANLCharacter::UnPossessed()
{
	AController* const OldController = Controller;

	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamID = TeamID;
	if (INLTeamAgentInterface* ControllerAsTeamProvider = Cast<INLTeamAgentInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();

	// Determine what the new team ID should be afterwards
	TeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
}

void ANLCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void ANLCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void ANLCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void ANLCharacter::InitializeGameplayTags()
{
}

void ANLCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent())
	{
		NLASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool ANLCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent())
	{
		return NLASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool ANLCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent())
	{
		return NLASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool ANLCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent())
	{
		return NLASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

USkeletalMeshComponent* ANLCharacter::GetPrimaryAttachmentMesh_Implementation() const
{
	return GetMesh();
}

void ANLCharacter::OnEliminationStarted(AActor*)
{
	DisableMovementAndCollision();
}

void ANLCharacter::OnEliminationFinished(AActor*)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToElimination);
}

void ANLCharacter::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UNLCharacterMovementComponent* NLMoveComp = CastChecked<UNLCharacterMovementComponent>(GetCharacterMovement());
	NLMoveComp->StopMovementImmediately();
	NLMoveComp->DisableMovement();
}

void ANLCharacter::DestroyDueToElimination()
{
	K2_OnEliminationFinished();

	UninitAndDestroy();
}

void ANLCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (UNLAbilitySystemComponent* NLASC = GetNLAbilitySystemComponent())
	{
		if (NLASC->GetAvatarActor() == this)
		{
			PawnExtComponent->UninitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}

void ANLCharacter::OnRep_ReplicatedAcceleration()
{
	if (UNLCharacterMovementComponent* NLMovementComponent = Cast<UNLCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel = NLMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]

		NLMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void ANLCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = TeamID;
			TeamID = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
		}
		else
		{
			UE_LOG(LogNLTeams, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogNLTeams, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

FGenericTeamId ANLCharacter::GetGenericTeamId() const
{
	return TeamID;
}

FOnNLTeamIndexChangedDelegate* ANLCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ANLCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId MyOldTeamID = TeamID;
	TeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, TeamID);
}

void ANLCharacter::OnRep_TeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
}
