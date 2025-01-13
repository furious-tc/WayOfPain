// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "EnhancedInputComponent.h"
#include "Input/NLInputConfig.h"
#include "NLInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;

USTRUCT()
struct FInputMappingContextAndPriority {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Input")
    TSoftObjectPtr<UInputMappingContext> InputMapping;

    // Higher priority input mappings will be prioritized over mappings with a lower priority.
    UPROPERTY(EditAnywhere, Category = "Input")
    int32 Priority = 0;

    /** If true, then this mapping context will be registered with the settings when this game feature action is registered. */
    UPROPERTY(EditAnywhere, Category = "Input")
    bool bRegisterWithSettings = true;
};

/**
 * UNLInputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config = Input)
class UNLInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:

	UNLInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const UNLInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
    void RemoveInputMappings(const UNLInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UNLInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UNLInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};

template<class UserClass, typename FuncType>
void UNLInputComponent::BindNativeAction(const UNLInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UNLInputComponent::BindAbilityActions(const UNLInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FNLInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}
}
