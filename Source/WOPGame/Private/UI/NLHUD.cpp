// Copyright 2025 Noblon GmbH. All Rights Reserved..

#include "UI/NLHUD.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Async/TaskGraphInterfaces.h"
#include "CommonUIExtensions.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLHUD)

class UWorld;

//////////////////////////////////////////////////////////////////////
// ANLHUD

ANLHUD::ANLHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ANLHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();

}

void ANLHUD::BeginPlay()
{
	Super::BeginPlay();

	AddWidgets();
}

void ANLHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWidgets();

	Super::EndPlay(EndPlayReason);
}

void ANLHUD::GetDebugActorList(TArray<AActor*>& InOutList)
{
	UWorld* World = GetWorld();

	Super::GetDebugActorList(InOutList);

	// Add all actors with an ability system component.
	for (TObjectIterator<UAbilitySystemComponent> It; It; ++It)
	{
		if (UAbilitySystemComponent* ASC = *It)
		{
			if (!ASC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			{
				AActor* AvatarActor = ASC->GetAvatarActor();
				AActor* OwnerActor = ASC->GetOwnerActor();

				if (AvatarActor && UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AvatarActor))
				{
					AddActorToDebugList(AvatarActor, InOutList, World);
				}
				else if (OwnerActor && UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
				{
					AddActorToDebugList(OwnerActor, InOutList, World);
				}
			}
		}
	}
}

void ANLHUD::AddWidgets()
{
	if (!GetOwningPlayerController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningPlayerController()->Player))
	{
		for (const FNLHUDLayoutRequest& Entry : Layout)
		{
			if (TSubclassOf<UCommonActivatableWidget> ConcreteWidgetClass = Entry.LayoutClass.Get())
			{
				LayoutsAdded.Add(UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, Entry.LayerID, ConcreteWidgetClass));
			}
		}

		UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
		for (const FNLHUDElementEntry& Entry : Widgets)
		{
			ExtensionHandles.Add(ExtensionSubsystem->RegisterExtensionAsWidgetForContext(Entry.SlotID, LocalPlayer, Entry.WidgetClass.Get(), -1));
		}
	}
}

void ANLHUD::RemoveWidgets()
{
	// Only unregister if this is the same HUD actor that was registered, there can be multiple active at once on the client
	for (TWeakObjectPtr<UCommonActivatableWidget>& AddedLayout : LayoutsAdded)
	{
		if (AddedLayout.IsValid())
		{
			AddedLayout->DeactivateWidget();
		}
	}

	for (FUIExtensionHandle& Handle : ExtensionHandles)
	{
		Handle.Unregister();
	}
}
