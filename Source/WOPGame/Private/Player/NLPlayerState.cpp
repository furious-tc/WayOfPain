// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#include "Player/NLPlayerState.h"
#include "Abilities/NLAbilitySet.h"
#include "Abilities/NLAbilitySystemComponent.h"
#include "Abilities/Attributes/NLCombatSet.h"
#include "Abilities/Attributes/NLHealthSet.h"
#include "Abilities/Attributes/NLMovementSet.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Pawns/NLPawnData.h"
#include "Pawns/NLPawnExtensionComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NLLogChannels.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLPlayerState)

const FName ANLPlayerState::NAME_NLAbilityReady("NLAbilitiesReady");

ANLPlayerState::ANLPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UNLAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	HealthSet = CreateDefaultSubobject<UNLHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UNLCombatSet>(TEXT("CombatSet"));
	MovementSet = CreateDefaultSubobject<UNLMovementSet>(TEXT("MovementSet"));

    // AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);

	TeamID = FGenericTeamId::NoTeam;
}

void ANLPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TeamID, SharedParams);

    SharedParams.Condition = ELifetimeCondition::COND_SkipOwner;
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, SharedParams);

	DOREPLIFETIME(ThisClass, StatTags);	
}

void ANLPlayerState::SetPawnData(const UNLPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (false && PawnData)
	{
		UE_LOG(LogNL, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	for (const UNLAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_NLAbilityReady);
	
	ForceNetUpdate();
}

void ANLPlayerState::OnRep_PawnData()
{
}

void ANLPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ANLPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void ANLPlayerState::Reset()
{
	Super::Reset();
}

void ANLPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (UNLPawnExtensionComponent* PawnExtComp = UNLPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckDefaultInitialization();
	}
}

void ANLPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

UAbilitySystemComponent* ANLPlayerState::GetAbilitySystemComponent() const
{
    return GetNLAbilitySystemComponent();
}

FRotator ANLPlayerState::GetReplicatedViewRotation() const
{
    // Could replace this with custom replication
    return ReplicatedViewRotation;
}

void ANLPlayerState::SetReplicatedViewRotation(const FRotator& NewRotation)
{
    if (NewRotation != ReplicatedViewRotation) {
        MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this);
        ReplicatedViewRotation = NewRotation;
    }
}

void ANLPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = TeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, TeamID, this);
		TeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogNLTeams, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId ANLPlayerState::GetGenericTeamId() const
{
	return TeamID;
}

FOnNLTeamIndexChangedDelegate* ANLPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ANLPlayerState::OnRep_TeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
}

void ANLPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void ANLPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 ANLPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool ANLPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void ANLPlayerState::ClientBroadcastMessage_Implementation(const FNLVerbMessage Message)
{
	// This check is needed to prevent running the action when in standalone mode
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}
