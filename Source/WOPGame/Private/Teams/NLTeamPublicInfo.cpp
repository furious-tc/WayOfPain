// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Teams/NLTeamPublicInfo.h"

#include "Net/UnrealNetwork.h"
#include "Teams/NLTeamInfoBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLTeamPublicInfo)

class FLifetimeProperty;

ANLTeamPublicInfo::ANLTeamPublicInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ANLTeamPublicInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, TeamDisplayAsset, COND_InitialOnly);
}

void ANLTeamPublicInfo::SetTeamDisplayAsset(TObjectPtr<UNLTeamDisplayAsset> NewDisplayAsset)
{
	check(HasAuthority());
	check(TeamDisplayAsset == nullptr);

	TeamDisplayAsset = NewDisplayAsset;

	TryRegisterWithTeamSubsystem();
}

void ANLTeamPublicInfo::OnRep_TeamDisplayAsset()
{
	TryRegisterWithTeamSubsystem();
}

