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

UENUM(BlueprintType)
namespace ECustomMovementMode{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode")
	};
}

UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	//~ Begin UCharacterMovementComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode,
		uint8 PreviousCustomMode) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual float GetMaxSpeed() const override;

	virtual float GetMaxAcceleration() const override;
	//~ End UCharacterMovementComponent Interface

public:
	/** Checks if character is currently climbing */
	bool IsClimbing() const;

	/** Toggles climbing state based on input */
	void ToggleToClimbing(bool bEnableClimb);

	FORCEINLINE FVector GetClimbableSurfaceNormal() { return CurrentClimbableSurfaceNormal; }

private:
	/** --------------------------------------------------------------------------
	 *  Climbing System Components
	 *  -------------------------------------------------------------------------- */

#pragma region ClimbTraces
	 /**
	 * Performs capsule-based multi-object tracing for climb surfaces
	 * @param Start - Trace start position
	 * @param End - Trace end position
	 * @param bShowDebug - Visualize debug shapes
	 * @param bPersistentDebug - Keep debug shapes persistent
	 */
	TArray<FHitResult> DoCapsuleTraceMultiByObject(
		const FVector& Start, 
		const FVector& End, 
		bool bShowDebugShape = false,
		bool bDrawPresistantShapes = false);

	/**
	 * Performs precise line trace for surface validation
	 * @param Start - Trace start position
	 * @param End - Trace end position
	 * @param bShowDebug - Visualize debug line
	 * @param bPersistentDebug - Keep debug line persistent
	 */
	FHitResult DoLineTraceSingleByObject(
		const FVector& Start,
		const FVector& End,
		bool bShowDebugShape = false,
		bool bDrawPresistantShapes = false);
#pragma endregion

#pragma region ClimbCore
private:
	/** Main surface detection routine */
	bool TraceClimbaleSurface();

	/**
	* Vertical surface scanning from eye level
	* @param TraceDistance - Maximum detection range (cm)
	* @param EyeHeightOffset - Vertical adjustment from base eye height (cm)
	*/
	FHitResult TraceFromEyeHeight(float TraceDistance, float EyeHeightOffset = 0.f);

	/** Determines if character can start climbing */
	bool CanStartClimbing();

	/** Enters climbing state */
	void StartClimbing();

	/** Exits climbing state */
	void StopClimbing();

	void PhysClimb(float deltaTime, int32 Iterations);

	void ProcessClimbaleSurfaceInfo();

	bool CheckShouldStopClimbing();

	FQuat GetClimbRotation(float DeltaTime);

	void SnapMovementToClimbableSurfaces(float DeltaTime);
#pragma endregion

#pragma region ClimbCoreVariables
	/** Results from last climbable surface detection */
	TArray<FHitResult> ClimbableSurfacesTraceResults;

	FVector CurrentClimbableSurfaceLocation;

	FVector CurrentClimbableSurfaceNormal;
#pragma endregion

#pragma region ClimbBPVariables
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Character Movement: Climbing",
		meta = (AllowPrivateAccess = "true"))
	float MaxBreakClimbDeceleration{400.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Character Movement: Climbing",
		meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed{100.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Character Movement: Climbing",
		meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration{300.f};

#pragma endregion

};
