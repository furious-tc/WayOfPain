// Copyright 2025 Noblon GmbH. All Rights Reserved..

#pragma once

#include "GameFramework/GameSession.h"
#include "NLGameSession.generated.h"

UCLASS(Config = Game)
class ANLGameSession : public AGameSession
{
	GENERATED_BODY()

public:

	ANLGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	/** Override to disable the default behavior */
	virtual bool ProcessAutoLogin() override;

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
};
