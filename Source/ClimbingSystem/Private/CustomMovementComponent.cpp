// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "ClimbingSystem/DebugHelper.h"

//~ Begin UCharacterMovementComponent Interface

void  UCustomMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

    if (OwningPlayerAnimInstance)
    {
        OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(
            this, 
            &UCustomMovementComponent::OnClimbMontageEnded
        );

        OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(
            this,
            &UCustomMovementComponent::OnClimbMontageEnded
        );
    }
}

void UCustomMovementComponent::TickComponent(
    float DeltaTime, 
    enum ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Core climbing detection update
   /* TraceClimbaleSurface();
    TraceFromEyeHeight(100.f);*/
    //CanClimbDownLedge();
}

// Called after very time movement mode changed
void UCustomMovementComponent::OnMovementModeChanged(
    EMovementMode PreviousMovementMode, 
    uint8 PreviousCustomMode)
{
    //// Transition TO climbing state
    if (IsClimbing())
    {
        bOrientRotationToMovement = false;
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);
    }

    // Transition FROM climbing state
    if (PreviousMovementMode == MOVE_Custom && 
        PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
    {
        bOrientRotationToMovement = true;
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(90.f);

        const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
        const FRotator CleanStandRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);
        UpdatedComponent->SetRelativeRotation(CleanStandRotation);

        StopMovementImmediately();
    }

    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    if (IsClimbing())
    {
        // Set up the custom physics
        PhysClimb(deltaTime, Iterations);
    }

    Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
    if (IsClimbing())
    {
        return MaxClimbSpeed;
    }
    else
    {
        return Super::GetMaxSpeed();
    }
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
    if (IsClimbing())
    {
        return MaxClimbAcceleration;
    }
    else
    {
        return Super::GetMaxAcceleration();
    }
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity,
    const FVector& CurrentVelocity) const
{
    const bool bIsPlayRMMontage{IsFalling() && 
        OwningPlayerAnimInstance && 
        OwningPlayerAnimInstance->IsAnyMontagePlaying()};

    if (bIsPlayRMMontage)
    {
        return RootMotionVelocity;
    }
	else
    {
        return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
    }
}

//~ End UCharacterMovementComponent Interface

void UCustomMovementComponent::ToggleToClimbing(bool bEnableClimb)
{
    if (bEnableClimb)
    {
        if (CanStartClimbing())
        {
            // Enter the climb state
            //Debug::Print(TEXT("Start Climbing..."), FColor::Green, 3);
            PlayClimbMontage(IdleToClimbMontage);
        }else
        {
            if (CanClimbDownLedge())
            {
                PlayClimbMontage(ClimbDownLedgeMontage);
                CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(40.f);
            }else
            {
                Debug::Print(TEXT("Cannot Climb Down the Ledge"), FColor::Red, 1);
            }
        }
    }
    else
    {
        // Stop climbing
        //Debug::Print(TEXT("StopClimbing..."), FColor::Green, 3);
        StopClimbing();
    }
}

// To check if we are in the state of climbing or not
bool UCustomMovementComponent::IsClimbing() const
{
    return MovementMode == MOVE_Custom && CustomMovementMode ==
        ECustomMovementMode::MOVE_Climb;
}


#pragma region ClimbTraces
// Get all objects in fron of character and out it in to an array and return this array.
TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(
    const FVector& Start, 
    const FVector& End, 
    bool bShowDebugShape,
    bool bDrawPresistantShapes)
{
    EDrawDebugTrace::Type DebugType{EDrawDebugTrace::None};

    if (bShowDebugShape)
    {
        DebugType = EDrawDebugTrace::ForOneFrame;
        if (bDrawPresistantShapes)
        {
            DebugType = EDrawDebugTrace::Persistent;
        }
    }

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
        DebugType,
        OutCapsuleTraceHitArry,
        false
    );

    return OutCapsuleTraceHitArry;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(
    const FVector& Start, 
    const FVector& End, 
    bool bShowDebugShape,
    bool bDrawPresistantShapes)
{
    EDrawDebugTrace::Type DebugType{EDrawDebugTrace::None};

    if (bShowDebugShape)
    {
        DebugType = EDrawDebugTrace::ForOneFrame;
        if (bDrawPresistantShapes)
        {
            DebugType = EDrawDebugTrace::Persistent;
        }
    }
    FHitResult Out;

    UKismetSystemLibrary::LineTraceSingleForObjects(
        this,
        Start,
        End,
        ClimbableSurfaceTraceTypes,
        false,
        TArray<AActor*>(),
        DebugType,
        Out,
        false
    );
    return Out;
}

#pragma endregion

#pragma region ClimbCore
// Core method for tracing surfaces,return true if near a climbable surface
bool UCustomMovementComponent::TraceClimbaleSurface()
{
    // Offset start position to prevent self-collision
    const FVector StartOffset{UpdatedComponent->GetForwardVector() * 30.f};
    const FVector Start{UpdatedComponent->GetComponentLocation() + StartOffset};
    const FVector End{Start + UpdatedComponent->GetForwardVector()};

    // Create capsule to detect climble suefaces in front
    ClimbableSurfacesTraceResults = DoCapsuleTraceMultiByObject(Start, End, false);

    return !ClimbableSurfacesTraceResults.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(
    float TraceDistance,
    float TraceStartOffset)
{
    // Calculate start position at character eye
    const FVector HeightOffset{UpdatedComponent->GetUpVector() *
        (TraceStartOffset + CharacterOwner->BaseEyeHeight)};

    const FVector Start{UpdatedComponent->GetComponentLocation() +
        HeightOffset};
    const FVector End{Start + UpdatedComponent->GetForwardVector() *
        TraceDistance};

    // Create line trace to detect surface
    return DoLineTraceSingleByObject(Start, End, true);
}

// To check if player can climb, return true if yes
bool UCustomMovementComponent::CanStartClimbing()
{
    // Must be on ground and facing climbale surface
    if (IsFalling()) return false;
    if (!TraceClimbaleSurface()) return false;

    //Must have overhead clearance
    if (!TraceFromEyeHeight(90.f).bBlockingHit) return false;

    return true;
}

bool UCustomMovementComponent::CanClimbDownLedge()
{
    if (IsFalling()) return false;

    const FVector ComponentLocation{UpdatedComponent->GetComponentLocation()};
    const FVector ComponentForwardVector{UpdatedComponent->GetForwardVector()};
    const FVector DownVector{-UpdatedComponent->GetUpVector()};

    const FVector WalkableSurfaceTraceStart{ComponentLocation + ComponentForwardVector * ClimbDownSurfaceTraceOffset};
    const FVector WalkableSurfaceTraceEnd{WalkableSurfaceTraceStart + DownVector * 100.f};

    FHitResult WalkableSurfaseHit{DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, true)};

    const FVector LedgeTraceStart{WalkableSurfaseHit.TraceStart + ComponentForwardVector * ClimbDownLedgeSurfaceTraceOffset};
    const FVector LedgeTraceEnd{LedgeTraceStart + DownVector * 200.f};

    FHitResult LedgeTraceHit = DoLineTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd, true);

    if (WalkableSurfaseHit.bBlockingHit && !LedgeTraceHit.bBlockingHit) return true;

    return false;
}

void UCustomMovementComponent::StartClimbing()
{
    SetMovementMode(MOVE_Custom,ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
    SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
    if (deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    /** Process all the climbable surfaces info */
    TraceClimbaleSurface();
    ProcessClimbaleSurfaceInfo();

    if (CheckHasReachedFloor())
    {
        Debug::Print(TEXT("HasReachedFloor..."), FColor::Green, 2);
    }else
    {
        Debug::Print(TEXT("HasNotReachedFloor..."), FColor::Red, 2);
    }

    // Check if we should stop climbing
    if (CheckShouldStopClimbing())
    {
        StopClimbing();
    }

    RestorePreAdditiveRootMotionVelocity();

    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
    // Define the max speed and acceleration
        CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
    }

    ApplyRootMotionToVelocity(deltaTime);

    FVector OldLocation = UpdatedComponent->GetComponentLocation();
    const FVector Adjusted = Velocity * deltaTime;
    FHitResult Hit(1.f);

    /** Handle climb rotation */
    SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

    if (Hit.Time < 1.f)
    {
        //adjust and try again
        HandleImpact(Hit, deltaTime, Adjusted);
        SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
    }

    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
    }

    /** Snap to climbable surface */
    SnapMovementToClimbableSurfaces(deltaTime);

    if (CheckHasReachedLedge())
    {
        Debug::Print(TEXT("HasReachedLedge"), FColor::Green, 1);
        PlayClimbMontage(ClimbToTopMontage);
    }else
    {
        Debug::Print(TEXT("HasNotReachedLedge"), FColor::Red, 1);
    }
}

void UCustomMovementComponent::ProcessClimbaleSurfaceInfo()
{
    CurrentClimbableSurfaceLocation = FVector::ZeroVector;
    CurrentClimbableSurfaceNormal = FVector::ZeroVector;

    if (ClimbableSurfacesTraceResults.IsEmpty()) return;

    for (const FHitResult& TraceHitResult : ClimbableSurfacesTraceResults)
    {
        CurrentClimbableSurfaceLocation += TraceHitResult.ImpactPoint;
        CurrentClimbableSurfaceNormal += TraceHitResult.ImpactNormal;
    }

    CurrentClimbableSurfaceLocation /= ClimbableSurfacesTraceResults.Num();
    CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();
}

bool UCustomMovementComponent::CheckShouldStopClimbing()
{
    if (ClimbableSurfacesTraceResults.IsEmpty()) return true;

    const float DotProductOfUpVectorAndSurfaceNormal{
        static_cast<float>(FVector::DotProduct(CurrentClimbableSurfaceNormal,
            FVector::UpVector))};

    const float Degree{FMath::RadiansToDegrees(
        FMath::Acos(DotProductOfUpVectorAndSurfaceNormal))};

    if (Degree <= 60.f) return true;

    Debug::Print(TEXT("Degree of Climb Surface: ") + FString::SanitizeFloat(Degree),FColor::Yellow,1);

    return false;
}

bool UCustomMovementComponent::CheckHasReachedFloor()
{
    // If Character is Climbing Up,means he is not going to reach floor.
    if (GetUnrotatedClimbVelocity().Z > 10.f) return false;

    const FVector DownVector{-UpdatedComponent->GetUpVector()};
    const FVector StartOffset{DownVector * 50.f};

    const FVector Start{UpdatedComponent->GetComponentLocation() + StartOffset};
    const FVector End{Start + DownVector};

    TArray<FHitResult> PossibleFloorHits{
        DoCapsuleTraceMultiByObject(Start, End, true)};

    if (PossibleFloorHits.IsEmpty()) return false;

    for (const FHitResult& PossibleFloorHit : PossibleFloorHits)
    {
        const bool bFloorReached = FVector::Parallel(
            -PossibleFloorHit.ImpactNormal, FVector::UpVector) &&
            GetUnrotatedClimbVelocity().Z < -10.f;

        if (bFloorReached)
        {
            return true;
        }
    }
    return false;
}

bool UCustomMovementComponent::CheckHasReachedLedge()
{
    FHitResult LedgeHitResult = 
        TraceFromEyeHeight(100.f, 50.f);

    if (!LedgeHitResult.bBlockingHit)
    {
        const FVector WalkableSurfaceTraceStart{LedgeHitResult.TraceEnd};

        const FVector DownVector{-UpdatedComponent->GetUpVector()};
        const FVector WalkableSurfaceTraceEnd{WalkableSurfaceTraceStart + DownVector * 100.f};
        FHitResult WalkableSurfaceHitResult =
            DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd, true);

        if (WalkableSurfaceHitResult.bBlockingHit && GetUnrotatedClimbVelocity().Z > 10.f)
        {
            return true;
        }
    }

    return false;
}

FQuat UCustomMovementComponent::GetClimbRotation(float DeltaTime)
{
    const FQuat CurrentQuat{UpdatedComponent->GetComponentQuat()};
    
    if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
    {
        return CurrentQuat;
    }

    FQuat TargetQuat{FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat()};

    return FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.f);
}

void UCustomMovementComponent::SnapMovementToClimbableSurfaces(float DeltaTime)
{
    const FVector ComponentLocation{UpdatedComponent->GetComponentLocation()};
    const FVector ComponentFowrard{UpdatedComponent->GetForwardVector()};

    const FVector ProjectedCharacterToSurface{
        (CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentFowrard)
    };

    const FVector SnapVector{-CurrentClimbableSurfaceNormal * 
        ProjectedCharacterToSurface.Length()};

    UpdatedComponent->MoveComponent(
        SnapVector * DeltaTime * MaxClimbSpeed,
        UpdatedComponent->GetComponentQuat(),
        true);
}

void UCustomMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
    if (!MontageToPlay) return;
    if (!OwningPlayerAnimInstance) return;
    if (OwningPlayerAnimInstance->IsAnyMontagePlaying()) return;

    OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
}

void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    Debug::Print(TEXT("Climb Montage Ended..."));
    if (Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
    {
        StartClimbing();
        StopMovementImmediately();
    }
    if (Montage == ClimbToTopMontage)
    {
        SetMovementMode(MOVE_Walking);
    }
}

FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
    return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}
#pragma endregion
