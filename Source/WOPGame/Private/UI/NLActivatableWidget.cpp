// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "UI/NLActivatableWidget.h"

#include "Editor/WidgetCompilerLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLActivatableWidget)

#define LOCTEXT_NAMESPACE "NL"

UNLActivatableWidget::UNLActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> UNLActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
	case ENLWidgetInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
	case ENLWidgetInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
	case ENLWidgetInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	case ENLWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}

#if WITH_EDITOR

void UNLActivatableWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	if (!GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UNLActivatableWidget, BP_GetDesiredFocusTarget)))
	{
		if (GetParentNativeClass(GetClass()) == UNLActivatableWidget::StaticClass())
		{
			CompileLog.Warning(LOCTEXT("ValidateGetDesiredFocusTarget_Warning", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen."));
		}
		else
		{
			//TODO - Note for now, because we can't guarantee it isn't implemented in a native subclass of this one.
			CompileLog.Note(LOCTEXT("ValidateGetDesiredFocusTarget_Note", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen.  If it was implemented in the native base class you can ignore this message."));
		}
	}
}

#endif

#undef LOCTEXT_NAMESPACE
