// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TraceClimbaleSurface();
    TraceFromEyeHeight(100.f);
}


#pragma region ClimbTraces

// Get all objects in fron of character and out it in to a array and reture this array.
TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape)
{
    TArray<FHitResult> OutCapsuleTraceHitArry;
    UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        this,
        Start,
        End,
        ClimbCapsuleTraceRadius,
        ClimbCapsuleTraceHalfHeight,
        ClimbableSurfaceTraceTypes,
        false,
        TArray<AActor*>(),
        bShowDebugShape ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
        OutCapsuleTraceHitArry,
        false
    );

    return OutCapsuleTraceHitArry;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape)
{
    FHitResult Out;

    UKismetSystemLibrary::LineTraceSingleForObjects(
        this,
        Start,
        End,
        ClimbableSurfaceTraceTypes,
        false,
        TArray<AActor*>(),
        bShowDebugShape ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
        Out,
        false
    );
    return Out;
}

#pragma endregion

#pragma region ClimbTraceCore

// Core metiod for tracing surfaces.
void  UCustomMovementComponent::TraceClimbaleSurface()
{
    const FVector StartOffset{UpdatedComponent->GetForwardVector() * 30.f};
    const FVector Start{UpdatedComponent->GetComponentLocation() + StartOffset};
    const FVector End{Start + UpdatedComponent->GetForwardVector()};

    DoCapsuleTraceMultiByObject(Start, End, true);
}

void UCustomMovementComponent::TraceFromEyeHeight(const float& TraceDistance, const float& TraceStartOffset)
{
    const FVector HeightOffset{UpdatedComponent->GetUpVector() * CharacterOwner->BaseEyeHeight + TraceStartOffset};
    const FVector Start{UpdatedComponent->GetComponentLocation() + HeightOffset};
    const FVector End{Start + UpdatedComponent->GetForwardVector() * TraceDistance};

    DoLineTraceSingleByObject(Start, End, true);
}

#pragma endregion
