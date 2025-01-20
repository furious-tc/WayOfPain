// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "GameUIManagerSubsystem.h"

#include "NLUIManagerSubsystem.generated.h"

class FSubsystemCollectionBase;
class UObject;

UCLASS()
class UNLUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()

public:

	UNLUIManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaTime);
	void SyncRootLayoutVisibilityToShowHUD();
	
	FTSTicker::FDelegateHandle TickHandle;
};
