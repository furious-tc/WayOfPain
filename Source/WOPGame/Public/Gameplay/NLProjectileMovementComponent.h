// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NLProjectileMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNLHomingTargetActorClearedDelegate, AActor*, Owner, AActor*, ClearedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNLHomingTargetReachedDelegate, USceneComponent*, HomingTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNLHomingTargetChangedDelegate, USceneComponent*, HomingTarget);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNLHomingTargetReached, USceneComponent* /*HomingTarget*/);

/**
 * ProjectileMovementComponent version for Way of Pain projectiles.
 */
UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent), ShowCategories = (Velocity))
class WOPGAME_API UNLProjectileMovementComponent : public UProjectileMovementComponent {
    GENERATED_UCLASS_BODY()

public:
    /** Compute the acceleration that will be applied */
    virtual FVector ComputeAcceleration(const FVector& InVelocity, float DeltaTime) const override;

    /** Allow the projectile to track towards its homing target. */
    virtual FVector ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const override;

    UFUNCTION(BlueprintCallable, Category = NL)
    virtual void SetHomingTarget(USceneComponent* InComponent, FVector InHomingOffset, FName InAttachSocket);
    
    UFUNCTION(BlueprintCallable, Category = NL)
    virtual AActor* GetHomingTargetActor() const;

    UFUNCTION(BlueprintCallable, Category = NL)
    const FVector GetCalculatedHomingTargetLocation() const;

protected:
    UFUNCTION()
    virtual void OnHomingTargetActorCleared(AActor* ClearedActor);

    UFUNCTION()
    void HandleNLHomingTargetReached(USceneComponent* HomingTarget);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NL)
    FVector HomingTargetOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NL)
    bool bAttachToHomingTargetOnReach = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NL)
    bool bUseProxyOffsetOnAttachToHoming = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NL)
    float HomingTargetReachDistance = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NL)
    FName AttachSocket;

    // Delegate called on homing target is cleared (destroyed/eliminated/etc)
    UPROPERTY(BlueprintAssignable, Category = NL)
    FOnNLHomingTargetActorClearedDelegate OnHomingTargetActorClearedDelegate;

    // Delegate called on homing target is reached
    UPROPERTY(BlueprintAssignable, Category = NL)
    FOnNLHomingTargetReachedDelegate OnHomingTargetReachedDelegate;

    // Delegate called on homing target is changed
    UPROPERTY(BlueprintAssignable, Category = NL)
    FOnNLHomingTargetChangedDelegate OnHomingTargetChangedDelegate;

    mutable FOnNLHomingTargetReached OnNLHomingTargetReached;

};
