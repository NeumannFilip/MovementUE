// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SurferMovementComponent.generated.h"

/**
 * 
 */




UCLASS()
class SPEEDGAM340_API USurferMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	

public:
	
	
	//Overriding function taken from https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/Networking/CharacterMovementComponent/
	USurferMovementComponent();

	virtual void InitializeComponent() override;
	//Called when a component is registered https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Components/UActorComponent/OnRegister/
	//This ensures that proper movement component is taken into considerations
	void OnRegister() override;

	//overrides for custom movements
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//check for this in CharacterMovementComponent.cpp
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	//To calculate Braking  aka friction and slowing
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDecelarion) override;
	//Doing logic when in air
	void PhysFalling(float deltaTime, int32 Iterations);
	//restricting movement in air
	bool ShouldLimitAirControl(float DeltaTime, const FVector& FallAcceleration) const override;
	//Continues with new fall velocity.
	FVector NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const override;
	//Update the character state in PerformMovement right before doing the actual position change
	void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	//Update the character state in PerformMovement right after doing the actual position change
	void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

	//Surface friction
	void UpdateSurfaceFriction(bool bIsSliding = false);
	
	//Leaving the crouch since its just horrible
	/*
	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;
	virtual void DoCrouchResize(float TargetTime, float DeltaTime, bool bClientSimulation = false);
	virtual void DoUnCrouchResize(float TargetTime, float DeltaTime, bool bClientSimulation = false);
	*/

	//Jumping logic 
	bool CanAttemptJump() const override;
	bool DoJump(bool bClientSimulation) override;

	//allows upwards slides when walking if the surface is walkable
	void TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const override;
	//handles different movement modes separately; namely during walking physics we might not want to slide up slopes.
	float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact = false) override;

	//Slopes and Slides and Landing spots h
	//Calculate slide vector along a surface 
	//CharacterMoveComponent.h
	FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;
	FVector HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;
	//Decide if should go into falling mode when walking and changing position
	bool ShouldCatchAir(const FFindFloorResult& OldFloor, const FFindFloorResult& NewFloor) override;

	//Detecting tolerance for impact spots
	bool IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const override;

	//checker is the valid spot beneath capsule to land
	bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const override;
	bool ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const override;

	//Trace floor is a very good function that I took directly from projectBorealis that make tracing floor really easy
	//Essentaily it checks channels of capsle and matches it accordingly with floor
	void TraceCharacterFloor(FHitResult& OutHit);

	//ForceinLine to ensure VS compiles it first in order to keep acceleration top priority
	FORCEINLINE FVector GetAcceleration() const {
		return Acceleration;
	}

	//UFUNCTION(BlueprintCallable)
	//	bool IsOnLadder() const {
	//	return bOnLadder;
	//}

	//Calling after mode change
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);

	//
	float CameraBehaviour();

	//noclip for cheat
	void SetNoClip(bool bNoClip);
	//Toggglin noclip
	void ToggleNoClip();

	//Check for slowing down
	bool IsFrameBraking() const {
		return bFrameForBraking;
	}

	//bool IsInCrouch() const {
	//	return bInCrouch;
	//}

	//overriding max speed
	virtual float GetMaxSpeed() const override;

	//Show pos for the data display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing")
		uint32 bShowPos : 1;

private:
	//Some values for character to mach
	float DefaultStepHeight;
	float DefaultWalkableFloorZ;
	float SurfaceFriction;


	//Change modes
	bool bDelayMovementMode;
	EMovementMode DelayMovementMode;


protected:

	//also irrelevent i think
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement")
		//bool bOnLadder;

	//float MoveSoundTime;


	//A-D behavior aka if character is doing step
	bool StepSide;

	//Properties for calcualting various movement behaviours\
	//Ground acceleration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing")
		float GroundAccelerationMultiplier;

	//Air Acceeleration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing")
		float AirAccelerationMulitplier;

	//Restricting speed in air
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing")
		float AirSpeedCap;

	//Minimal distance to walk on stuff
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing")
		float MinStepHeight;

	//Braking as applying friction in the frames.
	bool bFrameForBraking;

	//bool bCrouchFrameTolerated = false;

	
	//bool bIsInCrouchTransition = false;

	//bool bInCrouch;

	class ASurferCharacter* SurferCharacter;

	//Setting up properties for stats like speed etc.


	//Walk speed when you hold shift 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Movement", meta = (ClampMin = "0", UIMin = "0"))
		float WalkSpeed;
	//Run is a default mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Movement", meta = (ClampMin = "0", UIMin = "0"))
		float RunSpeed;

	//Sprint mode when exceeding speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Movement", meta = (ClampMin = "0", UIMin = "0"))
		float SprintSpeed;

	//i think irrelevant
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Movement", meta = (ClampMin = "0", UIMin = "0"))
		//float LadderSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Camera")
		float RollAngle = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Camera")
		float RollSpeed = 0.0f;

	// Minimum speed to scale up from for slope movement  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Camera", meta = (ClampMin = "0", UIMin = "0"))
		float MinimalSpeedMultiplier;

	// Max speed to scale up from for slope movement 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Movement", meta = (ClampMin = "0", UIMin = "0"))
		float MaximalSpeedMultiplier;

	//Speed of rolling the camera 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Camera")
		float CameraShakeMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfing x Camera", meta = (ClampMin = "0", UIMin = "0"))
		float AxisSpeedLimit = 6666.6f;

	
	//bool bShouldPlayMoveSounds = true;

};
