// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "UI/NLActivatableWidget.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"

#include "NLHUDLayout.generated.h"

class UCommonActivatableWidget;
class UObject;
class UNLControllerDisconnectedScreen;

/**
 * UNLHUDLayout
 *
 *	Widget used to lay out the player's HUD (typically specified by an Add Widgets action in the experience)
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "NL HUD Layout", Category = "NL|HUD"))
class UNLHUDLayout : public UNLActivatableWidget
{
	GENERATED_BODY()

public:

	UNLHUDLayout(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	void HandleEscapeAction();

	/**
	 * The menu to be displayed when the user presses the "Pause" or "Escape" button 
	 */
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;

};
