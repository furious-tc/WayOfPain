// Copyright 2025 Noblon GmbH. All Rights Reserved.

#include "Input/NLAimSensitivityData.h"
#include "Settings/NLSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NLAimSensitivityData)

UNLAimSensitivityData::UNLAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SensitivityMap =
	{
		{ ENLGamepadSensitivity::Slow,			0.5f },
		{ ENLGamepadSensitivity::SlowPlus,		0.75f },
		{ ENLGamepadSensitivity::SlowPlusPlus,	0.9f },
		{ ENLGamepadSensitivity::Normal,		1.0f },
		{ ENLGamepadSensitivity::NormalPlus,	1.1f },
		{ ENLGamepadSensitivity::NormalPlusPlus,1.25f },
		{ ENLGamepadSensitivity::Fast,			1.5f },
		{ ENLGamepadSensitivity::FastPlus,		1.75f },
		{ ENLGamepadSensitivity::FastPlusPlus,	2.0f },
		{ ENLGamepadSensitivity::Insane,		2.5f },
	};
}

const float UNLAimSensitivityData::SensitivtyEnumToFloat(const ENLGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}

