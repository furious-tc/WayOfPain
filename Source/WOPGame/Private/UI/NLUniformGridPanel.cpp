// Copyright 2025 Noblon GmbH. All Rights Reserved. 


#include "UI/NLUniformGridPanel.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Components/UniformGridSlot.h"

int32 UNLUniformGridPanel::GetAutoMaxColumn() const
{
    return AutoMaxColumn;
}

void UNLUniformGridPanel::SetAutoMaxColumn(int32 InAutoMaxColumn)
{
    check(InAutoMaxColumn > 0);
    AutoMaxColumn = InAutoMaxColumn;
}

UUniformGridSlot* UNLUniformGridPanel::AutoAddChildToUniformGrid(UWidget* Content)
{
    UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(Super::AddChild(Content));

    check(AutoMaxColumn > 0);

    if (GridSlot != nullptr) {
        GridSlot->SetColumn(Slots.Num() % AutoMaxColumn);
        GridSlot->SetRow(Slots.Num() / AutoMaxColumn);
    }

    return GridSlot;
}

void UNLUniformGridPanel::AutoAssignColRowToSlots()
{
    int32 SlotIndex = 0;

    for (UPanelSlot* PanelSlot : Slots) {
        if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(PanelSlot)) {
            GridSlot->SetColumn(SlotIndex % AutoMaxColumn);
            GridSlot->SetRow(SlotIndex / AutoMaxColumn);
        }

        ++SlotIndex;
    }
}
