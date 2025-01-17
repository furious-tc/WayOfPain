// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Teams/NLTeamCheats.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Teams/NLTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLTeamCheats)

//////////////////////////////////////////////////////////////////////
// UNLTeamCheats

void UNLTeamCheats::CycleTeam()
{
	if (UNLTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<UNLTeamSubsystem>(GetWorld()))
	{
		APlayerController* PC = GetPlayerController();

		const int32 OldTeamId = TeamSubsystem->FindTeamFromObject(PC);
		const TArray<int32> TeamIds = TeamSubsystem->GetTeamIDs();
		
		if (TeamIds.Num())
		{
			const int32 IndexOfOldTeam = TeamIds.Find(OldTeamId);
			const int32 IndexToUse = (IndexOfOldTeam + 1) % TeamIds.Num();

			const int32 NewTeamId = TeamIds[IndexToUse];

			TeamSubsystem->ChangeTeamForActor(PC, NewTeamId);
		}

		const int32 ActualNewTeamId = TeamSubsystem->FindTeamFromObject(PC);

		UE_LOG(LogConsoleResponse, Log, TEXT("Changed to team %d (from team %d)"), ActualNewTeamId, OldTeamId);
	}
}

void UNLTeamCheats::SetTeam(int32 TeamID)
{
	if (UNLTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<UNLTeamSubsystem>(GetWorld()))
	{
		if (TeamSubsystem->DoesTeamExist(TeamID))
		{
			APlayerController* PC = GetPlayerController();

			TeamSubsystem->ChangeTeamForActor(PC, TeamID);
		}
	}
}

void UNLTeamCheats::ListTeams()
{
	if (UNLTeamSubsystem* TeamSubsystem = UWorld::GetSubsystem<UNLTeamSubsystem>(GetWorld()))
	{
		const TArray<int32> TeamIDs = TeamSubsystem->GetTeamIDs();

		for (const int32 TeamID : TeamIDs)
		{
			UE_LOG(LogConsoleResponse, Log, TEXT("Team ID %d"), TeamID);
		}
	}
}

