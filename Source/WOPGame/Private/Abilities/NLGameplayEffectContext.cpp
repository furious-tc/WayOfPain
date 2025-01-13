// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Abilities/NLGameplayEffectContext.h"
#include "Abilities/NLAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLGameplayEffectContext)

class FArchive;

FNLGameplayEffectContext* FNLGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FNLGameplayEffectContext::StaticStruct()))
	{
		return (FNLGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FNLGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

void FNLGameplayEffectContext::SetAbilitySource(const INLAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const INLAbilitySourceInterface* FNLGameplayEffectContext::GetAbilitySource() const
{
	return Cast<INLAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FNLGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

