// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "NLTeamInfoBase.h"

#include "NLTeamPublicInfo.generated.h"

class UNLTeamCreationComponent;
class UNLTeamDisplayAsset;
class UObject;
struct FFrame;

UCLASS()
class ANLTeamPublicInfo : public ANLTeamInfoBase
{
	GENERATED_BODY()

	friend UNLTeamCreationComponent;

public:
	ANLTeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UNLTeamDisplayAsset* GetTeamDisplayAsset() const { return TeamDisplayAsset; }

private:
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	void SetTeamDisplayAsset(TObjectPtr<UNLTeamDisplayAsset> NewDisplayAsset);

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamDisplayAsset)
	TObjectPtr<UNLTeamDisplayAsset> TeamDisplayAsset;
};
