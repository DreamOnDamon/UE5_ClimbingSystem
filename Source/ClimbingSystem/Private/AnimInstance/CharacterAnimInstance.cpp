// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/CharacterAnimInstance.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(TryGetPawnOwner());
    if (ClimbingSystemCharacter)
    {
        CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovement();
    }
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!ClimbingSystemCharacter || !CustomMovementComponent) return;

    SetGroundSpeed();
    SetAirSpeed();
    SetShouldMove();
    SetIsFalling();
    SetIsClimbing();
    SetClimbVelocity();
}

void UCharacterAnimInstance::SetGroundSpeed()
{
    GroundSpeed = UKismetMathLibrary::VSizeXY(
        ClimbingSystemCharacter->GetVelocity());
}

void UCharacterAnimInstance::SetAirSpeed()
{
    AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
}

void UCharacterAnimInstance::SetShouldMove()
{
    bShouldMove = 
        CustomMovementComponent->GetCurrentAcceleration().Size() > 0 &&
        GroundSpeed > 5.f &&
        !bIsFalling;
}

void UCharacterAnimInstance::SetIsFalling()
{
    bIsFalling = CustomMovementComponent->IsFalling();
}

void UCharacterAnimInstance::SetIsClimbing()
{
    bIsClimbing = CustomMovementComponent->IsClimbing();
}

void UCharacterAnimInstance::SetClimbVelocity()
{
    ClimbVelocity = CustomMovementComponent->GetUnrotatedClimbVelocity();
}
