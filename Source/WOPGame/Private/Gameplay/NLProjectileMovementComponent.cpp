// Copyright 2025 Noblon GmbH. All Rights Reserved. 

#include "Gameplay/NLProjectileMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UNLProjectileMovementComponent::UNLProjectileMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnNLHomingTargetReached.AddUObject(this, &ThisClass::HandleNLHomingTargetReached);
}

FVector UNLProjectileMovementComponent::ComputeAcceleration(const FVector& InVelocity, float DeltaTime) const
{
    if (bIsHomingProjectile && HomingTargetComponent.IsValid())
    {
        if (FVector::Dist(GetCalculatedHomingTargetLocation(), UpdatedComponent->GetComponentLocation()) <= HomingTargetReachDistance)
        {
            if (bAttachToHomingTargetOnReach)
            {
                USceneComponent* AttachTarget = HomingTargetComponent.Get();

                if (ACharacter* Char = Cast<ACharacter>(HomingTargetComponent->GetOwner())) {
                    AttachTarget = Char->GetMesh();
                }

                if (bUseProxyOffsetOnAttachToHoming)
                {
                    GetOwner()->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepWorldTransform, AttachSocket);
                }
                else
                {
                    GetOwner()->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocket);
                    GetOwner()->SetActorRelativeLocation(HomingTargetOffset);
                }
            }

            OnHomingTargetReachedDelegate.Broadcast(HomingTargetComponent.Get());
            OnNLHomingTargetReached.Broadcast(HomingTargetComponent.Get());
        }
    }

    return Super::ComputeAcceleration(InVelocity, DeltaTime);
}

// Allow the projectile to track towards its homing target.
FVector UNLProjectileMovementComponent::ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const
{
    FVector HomingAcceleration = ((GetCalculatedHomingTargetLocation() - UpdatedComponent->GetComponentLocation()).GetSafeNormal() * HomingAccelerationMagnitude);
    
    return HomingAcceleration;
}

void UNLProjectileMovementComponent::SetHomingTarget(USceneComponent* InComponent, FVector InHomingOffset, FName InAttachSocket)
{
    check(GetOwner());

    if (HomingTargetComponent != InComponent && HomingTargetComponent.IsValid())
    {
        if (HomingTargetComponent->GetOwner())
        {
            HomingTargetComponent->GetOwner()->OnDestroyed.RemoveAll(this);
        }

        if (GetOwner()->GetAttachParentActor())
        {
            GetOwner()->GetAttachParentActor()->OnDestroyed.RemoveAll(this);
        }
    }
    
    HomingTargetComponent = InComponent;
    OnHomingTargetChangedDelegate.Broadcast(InComponent);
    
    HomingTargetOffset = InHomingOffset;
    AttachSocket = InAttachSocket;

    if (HomingTargetComponent.IsValid())
    {
        GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        HomingTargetComponent->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnHomingTargetActorCleared);
        
        MaxSpeed = InitialSpeed;
        SetActive(true);
    }
    else
    {
        MaxSpeed = 0;
        SetActive(false);
    }
}

AActor* UNLProjectileMovementComponent::GetHomingTargetActor() const
{
    if (HomingTargetComponent.IsValid() && HomingTargetComponent->GetOwner()) {
        return HomingTargetComponent->GetOwner();
    }

    if (GetOwner()->GetAttachParentActor()) {
        return GetOwner()->GetAttachParentActor();
    }

    return nullptr;
}

const FVector UNLProjectileMovementComponent::GetCalculatedHomingTargetLocation() const
{
    FVector HomingTargetLocation = HomingTargetComponent.Get()->GetComponentLocation();

    if (ACharacter* Char = Cast<ACharacter>(HomingTargetComponent->GetOwner())) {
        HomingTargetLocation = Char->GetMesh()->GetSocketLocation(AttachSocket);
    }

    return HomingTargetLocation + HomingTargetOffset;
}

void UNLProjectileMovementComponent::HandleNLHomingTargetReached(USceneComponent* HomingTarget)
{
    HomingTargetComponent = nullptr;
    OnHomingTargetChangedDelegate.Broadcast(nullptr);

    MaxSpeed = 0;
    SetActive(false);
}

void UNLProjectileMovementComponent::OnHomingTargetActorCleared(AActor* ClearedActor)
{
    OnHomingTargetActorClearedDelegate.Broadcast(GetOwner(), ClearedActor);
}
