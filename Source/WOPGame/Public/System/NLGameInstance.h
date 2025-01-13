// Copyright 2025 Noblon GmbH. All Rights Reserved..

#pragma once

#include "Engine/GameInstance.h"
#include "NLGameInstance.generated.h"

UCLASS(Config = Game)
class WOPGAME_API UNLGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UNLGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

};
