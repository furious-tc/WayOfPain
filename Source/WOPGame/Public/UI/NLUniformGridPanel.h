// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Components/UniformGridPanel.h"
#include "NLUniformGridPanel.generated.h"

/**
 * NL Managed version of UniformGridPanel.
 */
UCLASS()
class WOPGAME_API UNLUniformGridPanel : public UUniformGridPanel
{
	GENERATED_BODY()
	
public:
    // Max column used for auto assigning items to relevant slot.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, BlueprintSetter = "SetAutoMaxColumn", Category = "NL|Child Layout")
    int32 AutoMaxColumn = 10;

public:
    UFUNCTION(BlueprintCallable, Category = "Child Layout")
    int32 GetAutoMaxColumn() const;

    UFUNCTION(BlueprintCallable, Category = "NL|Child Layout")
    void SetAutoMaxColumn(int32 InAutoMaxColumn);

    UFUNCTION(BlueprintCallable, Category = "NL|Widget")
    UUniformGridSlot* AutoAddChildToUniformGrid(UWidget* Content);

    UFUNCTION(BlueprintCallable, Category = "NL|Widget")
    void AutoAssignColRowToSlots();
};
