// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "System/GameplayTagStack.h"
#include "Templates/SubclassOf.h"
#include "NLGameplayItemInstance.generated.h"

class UNLGameplayItemDefinition;
class UNLGameplayItemFragment;
struct FGameplayTag;

/**
 * UNLGameplayItemInstance
 */
UCLASS(BlueprintType)
class UNLGameplayItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UNLGameplayItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~End of UObject interface

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=GameplayItem)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=GameplayItem)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=GameplayItem)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=GameplayItem)
	bool HasStatTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category=GameplayItem)
	TSubclassOf<UNLGameplayItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UNLGameplayItemFragment* FindFragmentByClass(TSubclassOf<UNLGameplayItemFragment> FragmentClass) const;

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

	void SetItemDef(TSubclassOf<UNLGameplayItemDefinition> InDef);

	friend struct FNLGameplayItemList;

private:
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	// The item definition
	UPROPERTY(Replicated)
    TSubclassOf<UNLGameplayItemDefinition> ItemDef;
};
