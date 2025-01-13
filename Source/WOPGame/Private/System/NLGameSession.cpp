// Copyright 2025 Noblon GmbH. All Rights Reserved..

#include "System/NLGameSession.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameSession)

ANLGameSession::ANLGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool ANLGameSession::ProcessAutoLogin()
{
	// This is actually handled in NLGameMode::TryDedicatedServerLogin
	return true;
}

void ANLGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void ANLGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

