// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

/**
 * Enhance movement component implementing advanced climbing mechanics
 * 
 * Handles surface dectection, climb validation and movement excution
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	//~ Begin UCharacterMovementComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UCharacterMovementComponent Interface

private:
	/** --------------------------------------------------------------------------
	 *  Climbing System Components
	 *  -------------------------------------------------------------------------- */

#pragma region ClimbTraces
	 /// Performs capsule-based multi-object tracing for climb surfaces
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape);

	/// Performs precise line trace for surface validation
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape);
#pragma endregion

#pragma region ClimbTraceCore
	/// Main surface detection routine
	void TraceClimbaleSurface();

	/// Vertical surface scanning from eye level
	/// @param TraceDistance - Maximum detection range (cm)
	/// @param EyeHeightOffset - Vertical adjustment from base eye height (cm)
	void TraceFromEyeHeight(float TraceDistance, float EyeHeightOffset = 0.f);
#pragma endregion

#pragma region ClimbVariables
	/** Object types considered climbable surfaces */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, 
		Category = "Character Movement: Climbing", 
		meta = (AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery> > ClimbableSurfaceTraceTypes;

	/** Radius of capsule used for climb detection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, 
		Category = "Character Movement: Climbing", 
		meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius{50.f};

	/** Half height of capsule used for climb detection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, 
		Category = "Character Movement: Climbing", 
		meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight{72.f};
#pragma endregion

};
