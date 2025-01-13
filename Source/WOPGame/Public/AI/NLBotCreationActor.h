// Copyright 2025 Noblon GmbH. All Rights Reserved.

#pragma once

#include "Components/SphereComponent.h"
#include "Abilities/NLAbilitySet.h"

#include "NLBotCreationActor.generated.h"

class UNLPawnData;
class AAIController;

/**
 * FNLBotCreationEntry
 *
 *	Data used to spawn bot.
 */
USTRUCT(BlueprintType)
struct FNLBotCreationEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Bot)
	TObjectPtr<const UNLPawnData> BotPawnData;

	UPROPERTY(EditAnywhere, Category=Bot)
	TSubclassOf<AAIController> BotControllerClass;
	
	UPROPERTY(EditAnywhere, Category=Bot)
	FString BotName;

	UPROPERTY(EditAnywhere, Category=Bot)
	int32 NumBotsToCreate = 1;

};

UCLASS(Blueprintable, BlueprintType)
class ANLBotCreationActor : public AActor
{
	GENERATED_BODY()

public:
	ANLBotCreationActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent;
#endif

protected:
	UPROPERTY(EditAnywhere, Category=Bot, meta=(TitleProperty= BotName))
	TArray<FNLBotCreationEntry> BotCreationEntries;

	UPROPERTY(EditAnywhere, Category = Bot)
	float ZoneRadius = 100.f;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAIController>> SpawnedBotList;

	UPROPERTY()
    FNLAbilitySet_GrantedHandles GrantedHandles;

#if WITH_SERVER_CODE

protected:
	virtual void ServerCreateBots();

#endif

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
