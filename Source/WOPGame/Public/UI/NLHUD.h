// Copyright 2025 Noblon GmbH. All Rights Reserved..

#pragma once

#include "GameFramework/HUD.h"
#include "CommonActivatableWidget.h"
#include "UIExtensionSystem.h"
#include "NLHUD.generated.h"

namespace EEndPlayReason { enum Type : int; }

struct FComponentRequestHandle;

USTRUCT()
struct FNLHUDLayoutRequest
{
	GENERATED_BODY()

	// The layout widget to spawn
	UPROPERTY(EditAnywhere, Category = UI, meta = (AssetBundles = "Client"))
	TSoftClassPtr<UCommonActivatableWidget> LayoutClass;

	// The layer to insert the widget in
	UPROPERTY(EditAnywhere, Category = UI, meta = (Categories = "UI.Layer"))
	FGameplayTag LayerID;
};


USTRUCT()
struct FNLHUDElementEntry
{
	GENERATED_BODY()

	// The widget to spawn
	UPROPERTY(EditAnywhere, Category = UI, meta = (AssetBundles = "Client"))
	TSoftClassPtr<UUserWidget> WidgetClass;

	// The slot ID where we should place this widget
	UPROPERTY(EditAnywhere, Category = UI)
	FGameplayTag SlotID;
};

/**
 * ANLHUD
 *
 *  Note that you typically do not need to extend or modify this class, instead you would
 *  use an "Add Widget" action in your game to add a HUD layout and widgets to it
 * 
 *  This class exists primarily for debug rendering
 */
UCLASS(Config = Game)
class ANLHUD : public AHUD
{
	GENERATED_BODY()

public:
	ANLHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UObject interface
	virtual void PreInitializeComponents() override;
	//~End of UObject interface

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AHUD interface
	virtual void GetDebugActorList(TArray<AActor*>& InOutList) override;
	//~End of AHUD interface


private:
	// Layout to add to the HUD
	UPROPERTY(EditAnywhere, Category = UI, meta = (TitleProperty = "{LayerID} -> {LayoutClass}"))
	TArray<FNLHUDLayoutRequest> Layout;

	// Widgets to add to the HUD
	UPROPERTY(EditAnywhere, Category = UI, meta = (TitleProperty = "{SlotID} -> {WidgetClass}"))
	TArray<FNLHUDElementEntry> Widgets;

private:

	TArray<TWeakObjectPtr<UCommonActivatableWidget>> LayoutsAdded;
	TArray<FUIExtensionHandle> ExtensionHandles;

	void AddWidgets();
	void RemoveWidgets();
};
