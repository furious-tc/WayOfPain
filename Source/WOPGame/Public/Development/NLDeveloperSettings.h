// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/SoftObjectPath.h"
#include "NLDeveloperSettings.generated.h"

struct FPropertyChangedEvent;

UENUM()
enum class ECheatExecutionTime
{
	// When the cheat manager is created
	OnCheatManagerCreated,

	// When a pawn is possessed by a player
	OnPlayerPawnPossession
};

USTRUCT()
struct FNLCheatToRun
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	ECheatExecutionTime Phase = ECheatExecutionTime::OnPlayerPawnPossession;

	UPROPERTY(EditAnywhere)
	FString Cheat;
};

/**
 * Developer settings / editor cheats
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UNLDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UNLDeveloperSettings();

	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface

public:
	// The experience override to use for Play in Editor (if not set, the default for the world settings of the open map will be used)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NL, meta=(AllowedTypes="NLExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NLBots, meta=(InlineEditConditionToggle))
	bool bOverrideBotCount = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NLBots, meta=(EditCondition=bOverrideBotCount))
	int32 OverrideNumPlayerBotsToSpawn = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NLBots)
	bool bAllowPlayerBotsToAttack = true;

	// Do the full game flow when playing in the editor, or skip 'waiting for player' / etc... game phases?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NL)
	bool bTestFullGameFlowInPIE = false;

	/**
	* Should force feedback effects be played, even if the last input device was not a gamepad?
	* The default behavior in NL is to only play force feedback if the most recent input device was a gamepad.
	*/
	UPROPERTY(config, EditAnywhere, Category = NL, meta = (ConsoleVariable = "NLPC.ShouldAlwaysPlayForceFeedback"))
	bool bShouldAlwaysPlayForceFeedback = false;

	// Should game logic load cosmetic backgrounds in the editor or skip them for iteration speed?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=NL)
	bool bSkipLoadingCosmeticBackgroundsInPIE = false;

	// List of cheats to auto-run during 'play in editor'
	UPROPERTY(config, EditAnywhere, Category=NL)
	TArray<FNLCheatToRun> CheatsToRun;
	
	// Should messages broadcast through the gameplay message subsystem be logged?
	UPROPERTY(config, EditAnywhere, Category=GameplayMessages, meta=(ConsoleVariable="GameplayMessageSubsystem.LogMessages"))
	bool LogGameplayMessages = false;

#if WITH_EDITORONLY_DATA
	/** A list of common maps that will be accessible via the editor detoolbar */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif
	
#if WITH_EDITOR
public:
	// Called by the editor engine to let us pop reminder notifications when cheats are active
	WOPGAME_API void OnPlayInEditorStarted() const;

private:
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
};
