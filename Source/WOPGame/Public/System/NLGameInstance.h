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

	virtual int32 AddLocalPlayer(ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;
	virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

private:
	/** This is the primary player*/
	TWeakObjectPtr<ULocalPlayer> PrimaryPlayer;
};
