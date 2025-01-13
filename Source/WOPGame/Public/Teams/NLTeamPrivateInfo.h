// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "NLTeamInfoBase.h"

#include "NLTeamPrivateInfo.generated.h"

class UObject;

UCLASS()
class ANLTeamPrivateInfo : public ANLTeamInfoBase
{
	GENERATED_BODY()

public:
	ANLTeamPrivateInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
