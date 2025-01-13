// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Cosmetics/NLControllerComponent_CharacterParts.h"
#include "Cosmetics/NLCharacterPartTypes.h"
#include "Cosmetics/NLPawnComponent_CharacterParts.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLControllerComponent_CharacterParts)

//////////////////////////////////////////////////////////////////////

UNLControllerComponent_CharacterParts::UNLControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNLControllerComponent_CharacterParts::BeginPlay()
{
	Super::BeginPlay();

	// Listen for pawn possession changed events
	if (HasAuthority())
	{
		if (AController* OwningController = GetController<AController>())
		{
			OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);

			if (APawn* ControlledPawn = GetPawn<APawn>())
			{
				OnPossessedPawnChanged(nullptr, ControlledPawn);
			}
		}
	}
}

void UNLControllerComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllCharacterParts();
	Super::EndPlay(EndPlayReason);
}

UNLPawnComponent_CharacterParts* UNLControllerComponent_CharacterParts::GetPawnCustomizer() const
{
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		return ControlledPawn->FindComponentByClass<UNLPawnComponent_CharacterParts>();
	}
	return nullptr;
}

void UNLControllerComponent_CharacterParts::AddCharacterPart(const FNLCharacterPart& NewPart)
{
	AddCharacterPartInternal(NewPart, ECharacterPartSource::Natural);
}

void UNLControllerComponent_CharacterParts::AddCharacterPartInternal(const FNLCharacterPart& NewPart, ECharacterPartSource Source)
{
	FNLControllerCharacterPartEntry& NewEntry = CharacterParts.AddDefaulted_GetRef();
	NewEntry.Part = NewPart;
	NewEntry.Source = Source;

	if (UNLPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		NewEntry.Handle = PawnCustomizer->AddCharacterPart(NewPart);
	}
}

void UNLControllerComponent_CharacterParts::RemoveCharacterPart(const FNLCharacterPart& PartToRemove)
{
	for (auto EntryIt = CharacterParts.CreateIterator(); EntryIt; ++EntryIt)
	{
		if (FNLCharacterPart::AreEquivalentParts(EntryIt->Part, PartToRemove))
		{
			if (UNLPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
			{
				PawnCustomizer->RemoveCharacterPart(EntryIt->Handle);
			}

			EntryIt.RemoveCurrent();
			break;
		}
	}
}

void UNLControllerComponent_CharacterParts::RemoveAllCharacterParts()
{
	if (UNLPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		for (FNLControllerCharacterPartEntry& Entry : CharacterParts)
		{
			PawnCustomizer->RemoveCharacterPart(Entry.Handle);
		}
	}

	CharacterParts.Reset();
}

void UNLControllerComponent_CharacterParts::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// Remove from the old pawn
	if (UNLPawnComponent_CharacterParts* OldCustomizer = OldPawn ? OldPawn->FindComponentByClass<UNLPawnComponent_CharacterParts>() : nullptr)
	{
		for (FNLControllerCharacterPartEntry& Entry : CharacterParts)
		{
			OldCustomizer->RemoveCharacterPart(Entry.Handle);
			Entry.Handle.Reset();
		}
	}

	// Apply to the new pawn
	if (UNLPawnComponent_CharacterParts* NewCustomizer = NewPawn ? NewPawn->FindComponentByClass<UNLPawnComponent_CharacterParts>() : nullptr)
	{
		for (FNLControllerCharacterPartEntry& Entry : CharacterParts)
		{
			Entry.Handle = NewCustomizer->AddCharacterPart(Entry.Part);
		}
	}
}
