// Fill out your copyright notice in the Description page of Project Settings.


#include "SurferMovementComponent.h"


#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Sound/SoundCue.h"

#include "SurferCharacter.h"

//Debug stuff
//static TAutoConsoleVariable<int32> CVarShowPos(TEXT("cl.ShowPos"), 0, TEXT("Show position and movement information.\n"), ECVF_Default);


//Here comes tons of links to documentation about various componennts and functons
/*
* https://docs.unrealengine.com/5.1/en-US/API/Runtime/Engine/Components/UCapsuleComponent/
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/BrakingFriction/
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/BrakingFrictionF-/
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/bUseSeparateBrak-/
*
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/CalcVelocity/ Very important
* https://docs.unrealengine.com/5.1/en-US/world-settings-in-unreal-engine/
* https://docs.unrealengine.com/4.26/en-US/TestingAndOptimization/PerformanceAndProfiling/StatCommands/StatsSystemOverview/
* https://docs.unrealengine.com/4.27/en-US/TestingAndOptimization/PerformanceAndProfiling/CSVProfiler/
* https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/HAL/IConsoleManager/
* https://docs.unrealengine.com/5.1/en-US/API/Runtime/Core/Math/FMath/
* https://docs.unrealengine.com/4.27/en-US/API/Runtime/Core/Math/FRotationMatrix/

*/

//Declares a cycle counter stat
DECLARE_CYCLE_STAT(TEXT("Surfer Beginning"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Surfer Falling physics"), STAT_CharPhysFalling, STATGROUP_Character);

//Setting the velocity the same as in source engine
constexpr float JumpVelocity = 266.7f;
//Max value between defecting step
const float MAX_STEP_SIDE_Z = 0.09f;
//Setting minimal value of vertical limit
const float VerticalSlop = 0.001f;
//Setting gravity the same as in source engine
constexpr float DesiredGravity = -1143.0f;

USurferMovementComponent::USurferMovementComponent()
{

	//Values that needs overriding:
	//These are based on the real commands and values in games such as Halflife or Counter Strike

	//cl_speed = 450 HU     There is a ratio between Unit in UE = 1.905 Unit in Source
	MaxAcceleration = 857.25f;

	WalkSpeed = 285.75f;
	RunSpeed = 361.9f;
	MaxWalkSpeed = RunSpeed;

	AirControl = 1.0f;

	AirControlBoostMultiplier = 0.0f;
	AirControlBoostVelocityThreshold = 0.0f;

	//30 air speed cap 
	AirSpeedCap = 57.15f;
	
	//sv_accelerate and sv_airaccelerate
	GroundAccelerationMultiplier = 10.0f;
	AirAccelerationMulitplier = 10.0f;


	//sv_friction
	GroundFriction = 4.0f;
	BrakingFriction = 4.0f;
	SurfaceFriction = 1.0f;
	bUseSeparateBrakingFriction = false;
	//Slowing facter
	BrakingFrictionFactor = 1.0f;
	//slowing factor detection range
	BrakingSubStepTime = 0.015f;

	
	//MaxSimulationTimeStep = 0.5f;
	//MaxSimulationIterations = 1;

	//sv_stopspeed in air
	FallingLateralFriction = 0.0f;
	BrakingDecelerationFalling = 0.0f;
	BrakingDecelerationFlying = 190.5f;

	BrakingDecelerationWalking = 190.5f;

	// HL2 step height
	MaxStepHeight = 34.29f;
	DefaultStepHeight = MaxStepHeight;
	
	MinStepHeight = 10.0f;

	
	// 21Hu jump height
	JumpZVelocity = 304.8f;
	// Don't bounce off characters
	JumpOffJumpZFactor = 0.0f;
	// Default show pos to false
	bShowPos = false;
	// We aren't on a ladder at first
	//bOnLadder = false;
	
	//LadderSpeed = 381.0f;
	// Speed multiplier bounds
	MinimalSpeedMultiplier = SprintSpeed * 1.7f;
	MaximalSpeedMultiplier = SprintSpeed * 2.5f;

	// Start out braking/slowing
	bFrameForBraking = true;


	// Max slope in source is 45.57
	SetWalkableFloorZ(0.5f);
	DefaultWalkableFloorZ = GetWalkableFloorZ();
	AxisSpeedLimit = 6667.5f;
	// Tune physics interactions
	StandingDownwardForceScale = 1.0f;



	
	
	
	// Flat base
	bUseFlatBaseForFloorChecks = true;
	
	// Make sure gravity is correct for player movement
	GravityScale = DesiredGravity / UPhysicsSettings::Get()->DefaultGravityZ;
	// Make sure ramp movement in correct
	bMaintainHorizontalGroundVelocity = true;

}


/// <summary>
/// Getting the surface friction on hit 
/// </summary>
/// <param name="Hit"></param>
/// <returns></returns>
float SurfaceFrictionHit(const FHitResult& Hit)
{
	//Assigning friction
	float SurfaceFriction = 1.0f;
	//Whenever material of the character is hitiing surface
	if (Hit.PhysMaterial.IsValid())
	{
		SurfaceFriction = FMath::Min(1.0f, Hit.PhysMaterial->Friction * 1.25f);
	}
	return SurfaceFriction;
}

/// <summary>
/// Intializing component with cast to owner
/// </summary>
void USurferMovementComponent::InitializeComponent()
{

	Super::InitializeComponent();
	SurferCharacter = Cast<ASurferCharacter>(GetOwner());
}

/// <summary>
/// Registering the proper movement component to the running game.
/// </summary>
void USurferMovementComponent::OnRegister()
{
	//CharacterMovementComponent.h 
	Super::OnRegister();

	//
	const bool bIsReplay = (GetWorld() && GetWorld()->IsPlayingReplay());
	if (!bIsReplay && GetNetMode() == NM_ListenServer) {
		NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
	}

}

/// <summary>
/// CalcVelocity is a function that handles entire logic and calculation of player behavior depenidn on the type of movement is being performed
/// O//ITs mostly coping from the charactermovementcomponent.cpp
/// </summary>
/// <param name="DeltaTime"></param>
/// <param name="Friction"></param>
/// <param name="bFluid"></param>
/// <param name="BrakingDeceleration"></param>
void USurferMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{

	// Do not update velocity when using root motion or when SimulatedProxy and not simulating root motion - SimulatedProxy are repped their Velocity
	if (!HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME || (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy && !bWasSimulatingRootMotion))
	{
		return;
	}
	//ITs mostly coping from the charactermovementcomponent.cpp
	Friction = FMath::Max(0.0f, Friction);
	const float MaxAccel = GetMaxAcceleration();
	float MaxSpeed = GetMaxSpeed();


	//
	if (bForceMaxAccel)
	{
		// Force acceleration at full speed.
		// In consideration order for direction: Acceleration, then Velocity, then Pawn's rotation.
		if (Acceleration.SizeSquared() > SMALL_NUMBER)
		{
			Acceleration = Acceleration.GetSafeNormal() * MaxAccel;
		}
		else
		{
			Acceleration = MaxAccel * (Velocity.SizeSquared() < SMALL_NUMBER ? UpdatedComponent->GetForwardVector() : Velocity.GetSafeNormal());
		}

		AnalogInputModifier = 1.0f;
	}


	// Slowing down plater
	const bool bZeroAcceleration = Acceleration.IsNearlyZero();
	const bool bIsGroundMove = IsMovingOnGround() && bFrameForBraking;

	// Apply friction
	if (bIsGroundMove)
	{
		//Do exceed speed whenever the palyer exceeds standard 883u/s
		const bool bVelocityOverMax = IsExceedingMaxSpeed(MaxSpeed);
		const FVector OldVelocity = Velocity;

		const float ActualBrakingFriction = (bUseSeparateBrakingFriction ? BrakingFriction : Friction) * SurfaceFriction;
		ApplyVelocityBraking(DeltaTime, ActualBrakingFriction, BrakingDeceleration);

		
		if (bVelocityOverMax && Velocity.SizeSquared() < FMath::Square(MaxSpeed) && FVector::DotProduct(Acceleration, OldVelocity) > 0.0f)
		{
			Velocity = OldVelocity.GetSafeNormal() * MaxSpeed;
		}
	}

	// Apply fluid friction
	if (bFluid)
	{
		Velocity = Velocity * (1.0f - FMath::Min(Friction * DeltaTime, 1.0f));
	}

	// Limit before switching acceleration
	Velocity.X = FMath::Clamp(Velocity.X, -AxisSpeedLimit, AxisSpeedLimit);
	Velocity.Y = FMath::Clamp(Velocity.Y, -AxisSpeedLimit, AxisSpeedLimit);


	

	
	//Applying acceleration when moving
	if (!bZeroAcceleration)
	{
		// Clamp acceleration to max speed
		Acceleration = Acceleration.GetClampedToMaxSize2D(MaxSpeed);
		// Friction affects our ability to change direction. This is only done for input acceleration, not path following.
		//That's why I create extra vector that tracks the direction and adjust speed
		const FVector AccelDir = Acceleration.GetSafeNormal2D();
		const float VelocityDirection = Velocity.X * AccelDir.X + Velocity.Y * AccelDir.Y;
		///Adding speed in air
		const float AddSpeed = (bIsGroundMove ? Acceleration : Acceleration.GetClampedToMaxSize2D(AirSpeedCap)).Size2D() - VelocityDirection;
		
		//Whenever player is gaining speed
		if (AddSpeed > 0.0f)
		{
			// Apply acceleration
			//getting the percenatage of a ground and air http://adrianb.io/2015/02/14/bunnyhop.html according to this 
			const float AccelerationMultiplier = bIsGroundMove ? GroundAccelerationMultiplier : AirAccelerationMulitplier;
			FVector CurrentAcceleration = Acceleration * AccelerationMultiplier * SurfaceFriction * DeltaTime;
			CurrentAcceleration = CurrentAcceleration.GetClampedToMaxSize2D(AddSpeed);
			Velocity += CurrentAcceleration;
		}
	}



	// Limit after switching acceleration
	Velocity.X = FMath::Clamp(Velocity.X, -AxisSpeedLimit, AxisSpeedLimit);
	Velocity.Y = FMath::Clamp(Velocity.Y, -AxisSpeedLimit, AxisSpeedLimit);

	const float SpeedSq = Velocity.SizeSquared2D();

	//Sliding on a sleep has to be detected with a Walkable floor
	//Also provided that the highspeed can be regained on sliding
	if ( SpeedSq <= MaxWalkSpeedCrouched * MaxWalkSpeedCrouched)
	{
		// If we're crouching or not sliding, just use max
		MaxStepHeight = DefaultStepHeight;
		SetWalkableFloorZ(DefaultWalkableFloorZ);
	}
	else
	{
		// Scale step/ramp height down the faster we go
		////Meaning that the faster we go the more " steps " we complete and that can cause problems
		// that s why I want to clamp it 
		float Speed = FMath::Sqrt(SpeedSq);
		float SpeedScale = (Speed - MinimalSpeedMultiplier) / (MaximalSpeedMultiplier - MinimalSpeedMultiplier);
		float SpeedMultiplier = FMath::Clamp(SpeedScale, 0.0f, 1.0f);
		SpeedMultiplier *= SpeedMultiplier;
		if (!IsFalling())
		{
			//If on ground take friction
			SpeedMultiplier = FMath::Max((1.0f - SurfaceFriction) * SpeedMultiplier, 0.0f);
		}
		//Adjusting floor to and camera
		MaxStepHeight = FMath::Lerp(DefaultStepHeight, MinStepHeight, SpeedMultiplier);
		SetWalkableFloorZ(FMath::Lerp(DefaultWalkableFloorZ, 0.9848f, SpeedMultiplier));
	}



}


/// <summary>
/// Braking allows to control how much friction is being applied whenever surfer is moving across the surface.
/// It essentially is supposed to apply velocity in the oposing direction.
/// I changed small bits from the original code from charactermovementcomponent.cpp
/// </summary>
/// <param name="DeltaTime"></param>
/// <param name="Friction"></param>
/// <param name="BrakingDeceleration"></param>
void USurferMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDecelarion)
{
	//Initializing check if correct stuff is assigned
	if (Velocity.IsNearlyZero(0.1f) || !HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	//new float that makes velocity the lenbght of 2 components
	const float Speed = Velocity.Size2D();

	const float FrictionFactor = FMath::Max(0.0f, BrakingFrictionFactor);
	//Here I do extra calculation to add extra calculation for braking depending on the maximum speed
	Friction = FMath::Max(0.0f, Friction * FrictionFactor);
	{
		BrakingDecelarion = FMath::Max(BrakingDecelarion, Speed);
	}
	BrakingDecelarion = FMath::Max(0.0f, BrakingDecelarion);
	const bool bZeroFriction = FMath::IsNearlyZero(Friction);
	const bool bZeroBraking = BrakingDecelarion == 0.0f;

	//if no friction adn no braking just do nothing
	if (bZeroFriction || bZeroBraking)
	{
		return;
	}

	const FVector OldVel = Velocity;

	// Ensure that braking isnt applied inconsistently on bad or slow frames
	float RemainingTime = DeltaTime;
	const float MaxTimeStep = FMath::Clamp(BrakingSubStepTime, 1.0f / 75.0f, 1.0f / 20.0f);

	// Decelerate to brake to a stop
	const FVector RevAccel = -Velocity.GetSafeNormal();
	while (RemainingTime >= MIN_TICK_TIME)
	{
		//Use for contanst decelaration
		const float Delta = (RemainingTime > MaxTimeStep ? FMath::Min(MaxTimeStep, RemainingTime * 0.5f) : RemainingTime);
		RemainingTime -= Delta;

		// apply friction and braking
		Velocity += (Friction * BrakingDecelarion * RevAccel) * Delta;

		// Don't reverse direction
		if ((Velocity | OldVel) <= 0.0f)
		{
			Velocity = FVector::ZeroVector;
			return;
		}
	}

	// Clamp to zero if nearly zero
	if (Velocity.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		Velocity = FVector::ZeroVector;
	}

}


/// <summary>
/// This handles logic of gravity and player behavior in air
/// Majority of the code is taken also directly form CharacterMovementComponent.cpp
/// </summary>
/// <param name="deltaTime"></param>
/// <param name="Iterations"></param>
void USurferMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{

	SCOPE_CYCLE_COUNTER(STAT_CharPhysFalling);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CharPhysFalling);

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
	FallAcceleration.Z = 0.f;
	const bool bHasLimitedAirControl = ShouldLimitAirControl(deltaTime, FallAcceleration);

	float remainingTime = deltaTime;
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	{
		Iterations++;
		float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		bJustTeleported = false;

		const FVector OldVelocityWithRootMotion = Velocity;

		RestorePreAdditiveRootMotionVelocity();

		const FVector OldVelocity = Velocity;

		// Apply input
		const float MaxDecel = GetMaxBrakingDeceleration();
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			// Compute Velocity
			{
				// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
				Velocity.Z = 0.f;
				CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
				Velocity.Z = OldVelocity.Z;
			}
		}

		// Compute current gravity
		const FVector Gravity(0.f, 0.f, GetGravityZ());
		float GravityTime = timeTick;

		// If jump is providing force, gravity may be affected.
		bool bEndingJumpForce = false;
		if (CharacterOwner->JumpForceTimeRemaining > 0.0f)
		{
			// Consume some of the force time. Only the remaining time (if any) is affected by gravity when bApplyGravityWhileJumping=false.
			const float JumpForceTime = FMath::Min(CharacterOwner->JumpForceTimeRemaining, timeTick);
			GravityTime = bApplyGravityWhileJumping ? timeTick : FMath::Max(0.0f, timeTick - JumpForceTime);

			// Update Character state
			CharacterOwner->JumpForceTimeRemaining -= JumpForceTime;
			if (CharacterOwner->JumpForceTimeRemaining <= 0.0f)
			{
				CharacterOwner->ResetJumpState();
				bEndingJumpForce = true;
			}
		}

		// Apply gravity
		Velocity = NewFallVelocity(Velocity, Gravity, GravityTime);
		
		//Here I have deleted as this do interfere with the behavior i want
		
		//DecayFormerBaseVelocity(timeTick);
		
		// See if we need to sub-step to exactly reach the apex. This is important for avoiding "cutting off the top" of the trajectory as framerate varies.
		//Whats intresting that it does not recognize NumJumpApexAttempts that in original movement. Thus I had to comment it outm, but it doesnt change anything
		//Also I overrived the values with mine
		//That way I do take velocitiez into considerations not the rootmotion velocity or forcejump.
		if (OldVelocity.Z > 0.f && Velocity.Z <= 0.f /* && NumJumpApexAttempts < MaxJumpApexAttemptsPerSimulation*/)
		{
			const FVector DerivedAccel = (Velocity - OldVelocity) / timeTick;
			if (!FMath::IsNearlyZero(DerivedAccel.Z))
			{
				const float TimeToApex = -OldVelocity.Z / DerivedAccel.Z;

				// The time-to-apex calculation should be precise, and we want to avoid adding a substep when we are basically already at the apex from the previous iteration's work.
				const float ApexTimeMinimum = 0.0001f;
				if (TimeToApex >= ApexTimeMinimum && TimeToApex < timeTick)
				{
					const FVector ApexVelocity = OldVelocity + DerivedAccel * TimeToApex;
					Velocity = ApexVelocity;
					Velocity.Z = 0.f; // Should be nearly zero anyway, but this makes apex notifications consistent.

					// We only want to move the amount of time it takes to reach the apex, and refund the unused time for next iteration.
					remainingTime += (timeTick - TimeToApex);
					timeTick = TimeToApex;
					Iterations--;
					//NumJumpApexAttempts++;
					//Also Deleted the part about motion source
				}
			}
		}

		//Aplies rootmotion to velocity
		ApplyRootMotionToVelocity(timeTick);

	

		// Compute change in position (using midpoint integration method).
		FVector Adjusted = 0.5f * (OldVelocityWithRootMotion + Velocity) * timeTick;

		// Special handling if ending the jump force where we didn't apply gravity during the jump.
		if (bEndingJumpForce && !bApplyGravityWhileJumping)
		{
			// We had a portion of the time at constant speed then a portion with acceleration due to gravity.
			// Account for that here with a more correct change in position.
			const float NonGravityTime = FMath::Max(0.f, timeTick - GravityTime);
			Adjusted = (OldVelocityWithRootMotion * NonGravityTime) + (0.5f * (OldVelocityWithRootMotion + Velocity) * GravityTime);
		}

		// Move
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		float LastMoveTimeSlice = timeTick;
		float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

		//No need for swimming
		if (Hit.bBlockingHit)
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				//I dont want to adjsut my Velocity in the way provided in charctermovementComponent.cpp

				// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
				if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
				{
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					FFindFloorResult FloorResult;
					FindFloor(PawnLocation, FloorResult, false);
					if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
					{
						remainingTime += subTimeTickRemaining;
						ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
						return;
					}
				}

				HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsFalling())
				{
					return;
				}

				// Limit air control based on what we hit.
				// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
				FVector VelocityNoAirControl = OldVelocity;
				FVector AirControlAccel = Acceleration;
				if (bHasLimitedAirControl)
				{
					// Compute VelocityNoAirControl
					{
						// Find velocity *without* acceleration.
						TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
						TGuardValue<FVector> RestoreVelocity(Velocity, OldVelocity);
						Velocity.Z = 0.f;
						CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
						VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
						VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, GravityTime);
					}

					const bool bCheckLandingSpot = false; // we already checked above.
					AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;
					const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
					Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);
				
				//Adding Step as these do count to my movement. 
				FVector DeltaStep = ComputeSlideVector(Velocity * timeTick, 1.f - Hit.Time, OldHitNormal, Hit);

				//Removed part about deflection

				// Compute velocity after deflection (only gravity component for RootMotion)
				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (DeltaStep / subTimeTickRemaining);
					Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
				}

				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
				{
					// Move in deflected direction.
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

					if (Hit.bBlockingHit)
					{
						// hit second wall
						LastMoveTimeSlice = subTimeTickRemaining;
						subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime += subTimeTickRemaining;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, LastMoveTimeSlice, Delta);

						// If we've changed physics mode, abort.
						if (!HasValidData() || !IsFalling())
						{
							return;
						}

						// Act as if there was no air control on the last move when computing new deflection.
						if (bHasLimitedAirControl && Hit.Normal.Z > VerticalSlop)
						{
							const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
							Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
						}

						FVector PreTwoWallDelta = Delta;
						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// Limit air control, but allow a slide along the second wall.
						if (bHasLimitedAirControl)
						{
							const bool bCheckLandingSpot = false; // we already checked above.
							const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

							// Only allow if not back in to first wall
							if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
							{
								Delta += (AirControlDeltaV * subTimeTickRemaining);
							}
						}

						// Compute velocity after deflection (only gravity component for RootMotion)
						if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
						{
							const FVector NewVelocity = (Delta / subTimeTickRemaining);
							Velocity = HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocityWithIgnoreZAccumulate() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
						}

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
						if (Hit.Time == 0.f)
						{
							// if we are stuck then try to side step
							FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
							if (SideDelta.IsNearlyZero())
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
							}
							SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
						}

						if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (RandomStream.FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}
			}
		}

		if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
	}


}

/// <summary>
/// Simple check for restricting movement in air
/// </summary>
/// <param name="DeltaTime"></param>
/// <param name="FallAcceleration"></param>
/// <returns></returns>
bool USurferMovementComponent::ShouldLimitAirControl(float DeltaTime, const FVector& FallAcceleration) const
{
	return false;
}
/// <summary>
/// Custom logic for tracking new velocity.
/// </summary>
/// <param name="InitialVelocity"></param>
/// <param name="Gravity"></param>
/// <param name="DeltaTime"></param>
/// <returns></returns>
FVector USurferMovementComponent::NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const
{
	//Here I assign the falling velocity depending on the provided function
	FVector FallVel = Super::NewFallVelocity(InitialVelocity, Gravity, DeltaTime);
	//Here its simply clamped based on axis
	FallVel.Z = FMath::Clamp(FallVel.Z, -AxisSpeedLimit, AxisSpeedLimit);
	//return
	return FallVel;
}


/// <summary>
/// Updating movement just before 
/// clamping velocity.z
/// </summary>
/// <param name="DeltaSeconds"></param>
void USurferMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
	Velocity.Z = FMath::Clamp(Velocity.Z, -AxisSpeedLimit, AxisSpeedLimit);
	//UpdateCrouching(DeltaSeconds);
}

/// <summary>
/// Updating movement just after
/// after means to add friciton since its in move
/// </summary>
/// <param name="DeltaSeconds"></param>
void USurferMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
	Velocity.Z = FMath::Clamp(Velocity.Z, -AxisSpeedLimit, AxisSpeedLimit);
	UpdateSurfaceFriction();
	//UpdateCrouching(DeltaSeconds, true);
}

/// <summary>
/// This handles surfer's friction depending on the type of surface he is standing.
/// Also handles sliding - in this case scale it down to allow surfer to stick on the surface that is sliding on. 
/// </summary>
/// <param name="bIsSliding"></param>
void USurferMovementComponent::UpdateSurfaceFriction(bool bIsSliding)
{
	//Check for mode of walking and if on floor
	if (!IsFalling() && CurrentFloor.IsWalkableFloor())
	{
		//If it results in hit the floor,
		//just apply friciton 
		FHitResult Hit;
		TraceCharacterFloor(Hit);
		SurfaceFriction = SurfaceFrictionHit(Hit);
	}
	else
	{//Otherwise just make so its a sliding movmeent 
		//that way just adjust friction to slide mode which is smaller
		const bool bPlayerControlsMovedVertically = Velocity.Z > JumpVelocity || Velocity.Z <= 0.0f || bCheatFlying;
		if (bPlayerControlsMovedVertically)
		{
			SurfaceFriction = 1.0f;
		}
		else if (bIsSliding)
		{
			SurfaceFriction = 0.25f;
		}
	}

}

//void USurferMovementComponent::UpdateCrouching(float DeltaTime, bool bOnlyUncrouch)
//{
//
//
//}

//void USurferMovementComponent::Crouch(bool bClientSimulation)
//{
//
//	
//}

//void USurferMovementComponent::UnCrouch(bool bClientSimulation)
//{
//}

//void USurferMovementComponent::DoCrouchResize(float TargetTime, float DeltaTime, bool bClientSimulation)
//{
//	
//}

//void USurferMovementComponent::DoUnCrouchResize(float TargetTime, float DeltaTime, bool bClientSimulation)
//{
//
	
//}


/// <summary>
/// This handles the logic for checking if player has ability to jump
/// It's simple if statement 
/// </summary>
/// <returns></returns>
bool USurferMovementComponent::CanAttemptJump() const
{
	//local boll for jumpitng
	bool bCanAttemptJump = IsJumpAllowed();
	//If on ground check for floors and return
	if (IsMovingOnGround())
	{
		//Assign Floor to 1z or take data from checking floor
		const float BeneathFloor = FVector(0.0f, 0.0f, 1.0f) | CurrentFloor.HitResult.ImpactNormal;
		const float WalkableFloor = GetWalkableFloorZ();
		//Assign bool dependable of the result of the code above and/or to calculate the value of it and allow to jump
		bCanAttemptJump &= (BeneathFloor >= WalkableFloor) || FMath::IsNearlyEqual(BeneathFloor, WalkableFloor);
	}

	return bCanAttemptJump;
}

/// <summary>
/// This handles the logic of the actual jump
/// I dont change much, I add extra checker of the velocity.
/// </summary>
/// <param name="bClientSimulation"></param>
/// <returns></returns>
bool USurferMovementComponent::DoJump(bool bClientSimulation)
{

	if (!bCheatFlying && CharacterOwner && CharacterOwner->CanJump())
	{
		//If player moves vertically dissallow jump
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			//Velocity z smaller assign velocity.Z to default value assigned in the surfercomponent
			if (Velocity.Z <= 0.0f)
			{
				Velocity.Z = JumpZVelocity;
			}
			//Otherwise add it
			else
			{
				Velocity.Z += JumpZVelocity;
			}
			//Lastly after jump change mode of movement to falling
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}

	return false;
}

void USurferMovementComponent::TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
	
}

/// <summary>
/// Important function for surfing. 
/// Just referencing original code  from CharacterMovementComponent.cpp
/// </summary>
/// <param name="Delta"></param>
/// <param name="Time"></param>
/// <param name="Normal"></param>
/// <param name="Hit"></param>
/// <param name="bHandleImpact"></param>
/// <returns></returns>
float USurferMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}

/// <summary>
/// Important function for surfing
/// Just referenccing original code from CharacterMovementComponent.cpp
/// </summary>
/// <param name="Delta"></param>
/// <param name="Time"></param>
/// <param name="Normal"></param>
/// <param name="Hit"></param>
/// <returns></returns>
FVector USurferMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	return Super::ComputeSlideVector(Delta, Time, Normal, Hit);
}


/// <summary>
/// Here is some extra logic matching my needs regardiung the boost on the slopes / angled surfaces
/// </summary>
/// <param name="SlideResult"></param>
/// <param name="Delta"></param>
/// <param name="Time"></param>
/// <param name="Normal"></param>
/// <param name="Hit"></param>
/// <returns></returns>
FVector USurferMovementComponent::HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	//check for flying
	if ( bCheatFlying)
	{
		return Super::HandleSlopeBoosting(SlideResult, Delta, Time, Normal, Hit);
	}
	//new value for angled surfaces
	const float AngledSurface = FMath::Abs(Hit.ImpactNormal.Z);
	FVector ImpactNormal;
	// If too extreme, use the more stable hit normal
	// Very simple checker if any of the surfaces are vertical of horizontal
	if (AngledSurface <= 0.001f || AngledSurface == 1.0f)
	{
		//then just do normal impact
		ImpactNormal = Normal;
	}
	else
	{
		//Impact with sweep
		ImpactNormal = Hit.ImpactNormal;
	}
	//On the other hand its constrained to plane use special impact
	if (bConstrainToPlane)
	{
		ImpactNormal = ConstrainNormalToPlane(ImpactNormal);
	}
	//Camera behacior on the slopes 
	// probably useless
	const float BounceCoefficient = 1.0f + CameraShakeMultiplier * (1.0f - SurfaceFriction);
	return (Delta - BounceCoefficient * Delta.ProjectOnToNormal(ImpactNormal)) * Time;
}


/// <summary>
/// This handles logic of detecting whenever player is in "space" or flying
/// //https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/ShouldCatchAir/
/// Whether Character should go into falling mode when walking and changing position, based on an old and new floor result (both of which are considered walkable). Default implementation always returns false.
/// Default is false. but I want some movement.
/// </summary>
/// <param name="OldFloor"></param>
/// <param name="NewFloor"></param>
/// <returns></returns>
bool USurferMovementComponent::ShouldCatchAir(const FFindFloorResult& OldFloor, const FFindFloorResult& NewFloor)
{
	//Scaling speed with friction
	const float SpeedMultiplier = MaximalSpeedMultiplier / Velocity.Size2D();
	// Get surface friction
	const float CurrentSurfaceFriction = SurfaceFrictionHit(OldFloor.HitResult);
	//check for trying to surf
	const bool bIsSurfing = CurrentSurfaceFriction * SpeedMultiplier < 0.5f;

	//As velocity is horizontal, on ramp surfer is >90 angle, as such its comes out as a negative cosinus.
	const float Slope = Velocity | OldFloor.HitResult.ImpactNormal;
	const bool bUpRamp = Slope < 0.0f;

	//check for the slope
	const float VerticalZ = NewFloor.HitResult.ImpactNormal.Z - OldFloor.HitResult.ImpactNormal.Z;
	const bool bDownRamp = VerticalZ >= 0.0f;

	//Leaving the ramp
	const float StrafeMovement = FMath::Abs(GetLastInputVector() | GetOwner()->GetActorRightVector());
	const bool bStrafingOffRamp = StrafeMovement > 0.0f;

	//conditions for moving or leacing on ramp
	const bool bDoesWantCatchAir = bUpRamp || bStrafingOffRamp;

	//if these are true - catch air
	if (bIsSurfing && bDownRamp && bDoesWantCatchAir)
	{
		return true;
	}

	return Super::ShouldCatchAir(OldFloor, NewFloor);
}

/// <summary>
///  Relevant to my code, but jsut refernce original code from CharacterMovementComponent.cpp
/// </summary>
/// <param name="CapsuleLocation"></param>
/// <param name="TestImpactPoint"></param>
/// <param name="CapsuleRadius"></param>
/// <returns></returns>
bool USurferMovementComponent::IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const
{
	return Super::IsWithinEdgeTolerance(CapsuleLocation, TestImpactPoint, CapsuleRadius);
}

/// <summary>
/// This handles the logic of keeping track of whats below the player
/// Verify that the supplied hit result is a valid landing spot when falling.
/// Taken from CharacterMovementComponent.cpp
/// </summary>
/// <param name="CapsuleLocation"></param>
/// <param name="Hit"></param>
/// <returns></returns>
bool USurferMovementComponent::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}
	// Skip some checks if penetrating. Penetration will be handled by the FindFloor call (using a smaller capsule)
	if (!Hit.bStartPenetrating)
	{
		// Reject unwalkable floor normals.
		if (!IsWalkable(Hit))
		{
			return false;
		}

		float PawnRadius, PawnHalfHeight;
		CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

		// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
		if (bUseFlatBaseForFloorChecks)
		{
			// Reject hits that are above our box
			const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + MAX_FLOOR_DIST;
			if ((Hit.ImpactNormal.Z < GetWalkableFloorZ() || Hit.ImpactNormal.Z == 1.0f) && Hit.ImpactPoint.Z > LowerHemisphereZ)
			{
				return false;
			}
		}
		else
		{
			// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
			const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + PawnRadius;
			if (Hit.ImpactPoint.Z >= LowerHemisphereZ)
			{
				return false;
			}
		}

		// Reject hits that are barely on the cusp of the radius of the capsule
		if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
		{
			return false;
		}
	}
	else
	{
		// Penetrating
		if (Hit.Normal.Z < KINDA_SMALL_NUMBER)
		{
			// Normal is nearly horizontal or downward, that's a penetration adjustment next to a vertical or overhanging wall. Don't pop to the floor.
			return false;
		}
	}
	FFindFloorResult FloorResult;
	FindFloor(CapsuleLocation, FloorResult, false, &Hit);
	if (!FloorResult.IsWalkableFloor())
	{
		return false;
	}
	
	//This part better handles the part of slope landing .
	//Checking for type of surfface and velocity and hit
	if (Hit.Normal.Z < 1.0f && (Velocity | Hit.Normal) < 0.0f)
	{
		//The value of hit slope
		FVector DetectSlopeVector = Velocity;
		
		//Whenever the DetectSlopeVector.Z add gravity and world values
		DetectSlopeVector.Z += 0.5f * GetGravityZ() * GetWorld()->GetDeltaSeconds();
		//Make it into consideration by the compute vector function
		DetectSlopeVector = ComputeSlideVector(DetectSlopeVector, 1.0f, Hit.Normal, Hit);

		
		//Restrain it so if its greater than predefined value just return false
		if (DetectSlopeVector.Z > JumpVelocity)
		{
			return false;
		}
	}
	return true;
}



/// <summary>
///  Important function for surfing
/// Just referenccing original code from CharacterMovementComponent.cpp
/// </summary>
/// <param name="DeltaTime"></param>
/// <param name="Delta"></param>
/// <param name="Hit"></param>
/// <returns></returns>
bool USurferMovementComponent::ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const
{
	return !bUseFlatBaseForFloorChecks && Super::ShouldCheckForValidLandingSpot(DeltaTime, Delta, Hit);
}


/// <summary>
///  
/// Floor detection and track function that keeps connection with aplayer through capsule depending on the channels
/// taken from project borealis
/// https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/FindFloor/
/// </summary>
/// <param name="OutHit"></param>
void USurferMovementComponent::TraceCharacterFloor(FHitResult& OutHit)
{
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CharacterFloorTrace), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);
	// must trace complex to get mesh phys materials
	CapsuleParams.bTraceComplex = true;
	// must get materials
	CapsuleParams.bReturnPhysicalMaterial = true;

	const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
	FVector StandingLocation = PawnLocation;
	StandingLocation.Z -= MAX_FLOOR_DIST * 10.0f;
	GetWorld()->SweepSingleByChannel(
		OutHit,
		PawnLocation,
		StandingLocation,
		FQuat::Identity,
		CollisionChannel,
		StandingCapsuleShape,
		CapsuleParams,
		ResponseParam
	);

}

/// <summary>
/// Handling movement mode to reference and change when needed: such as walking, running, flying(falling)
/// </summary>
/// <param name="PreviousMovementMode"></param>
/// <param name="PreviousCustomMode"></param>
void USurferMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	//It means that while when player is walking therer is a fraction of step
	// So i want to make change mode only whenever full step is done
	StepSide = false;

	//jump or land
	bool bJumped = false;

	//Taking previous mode 
	if (PreviousMovementMode == MOVE_Walking && MovementMode == MOVE_Falling)
	{
		bJumped = true;
	}

	////tracing floor
	FHitResult Hit;
	TraceCharacterFloor(Hit);
	//PlayJumpSound(Hit, bJumped);

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

}


/// <summary>
/// This hanldes the logic for moving camera in differnt dimensions that will match player velocity.
/// </summary>
/// <returns></returns>
float USurferMovementComponent::CameraBehaviour()
{
	//if no movement just do nothing
	if (RollSpeed == 0.0f || RollAngle == 0.0f)
	{
		return 0.0f;
	}
	//assigning values to velocity or roation of the character owner.
	//this is a value that will detect and recalculate velocity and matrix(3-dimensional) rotation
	float LookAround = Velocity | FRotationMatrix(GetCharacterOwner()->GetControlRotation()).GetScaledAxis(EAxis::Y);
	//Calculate it to Sign(float, returns - 1 if A < 0, 0 if A is zero, and +1 if A > 0) I use it as detector when velocity of camera changes
	const float Sign = FMath::Sign(LookAround);
	//Take the absolute value
	LookAround = FMath::Abs(LookAround);
	//if its small do it slowly
	if (LookAround < RollSpeed)
	{
		LookAround = LookAround * RollAngle / RollSpeed;
	}
	else//else match with roll angle
	{
		LookAround = RollAngle;
	}
	return LookAround * Sign;
}

void USurferMovementComponent::SetNoClip(bool bNoClip)
{



}

void USurferMovementComponent::ToggleNoClip()
{
	
}

/// <summary>
/// This handles logic that return the maxspeed depending on the mode of movement.
/// Also it restricts it and return the values
/// </summary>
/// <returns></returns>
float USurferMovementComponent::GetMaxSpeed() const
{//if flying ignore not implemented though
	if (bCheatFlying)
	{
		return (SurferCharacter->IsSprinting() ? SprintSpeed : WalkSpeed) * 1.5f;
	}

	// get speed and check on modes and correctly adjst depending on the mode
	//Its kind of changed version of the case logic in the original CharacterMovemtnComponent.cpp
	float Speed;
	if (SurferCharacter->IsSprinting())
	{
		if (IsCrouching() )
		{
			Speed = MaxWalkSpeedCrouched * 1.7f;
		}
		else
		{
			Speed = SprintSpeed;
		}
	}
	else if (SurferCharacter->DoesWantToWalk())
	{
		Speed = WalkSpeed;
	}
	else if (IsCrouching() )
	{
		Speed = MaxWalkSpeedCrouched;
	}
	else
	{
		Speed = RunSpeed;
	}

	return Speed;
}


/// <summary>
/// Here I am setting displaying of position and other data in tick to keep track of them
/// In terms of what happens. Its keeping track of mode of movement that character is performing
/// It makes character simulate physics 
/// Display data 
/// move camera and apply braking
/// </summary>
/// <param name="DeltaTime"></param>
/// <param name="TickType"></param>
/// <param name="ThisTickFunction"></param>
void USurferMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{


	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Check for movement modes
	if (bDelayMovementMode) {
		bDelayMovementMode = false;
		SetMovementMode(DelayMovementMode);
	}
	// keep updating physisc when neeeded
	if (UpdatedComponent->IsSimulatingPhysics()) {
		return;
	}

	
	//Displaying data
	GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::Red, FString::Printf(TEXT("position: %s"), *UpdatedComponent->GetComponentLocation().ToCompactString()));
	GEngine->AddOnScreenDebugMessage(2, 1.0f, FColor::Blue, FString::Printf(TEXT("angle: %s"), *CharacterOwner->GetControlRotation().ToCompactString()));
	GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Yellow, FString::Printf(TEXT("velocity: %f"), Velocity.Size()));
	
	//Keep control of the rotation and camera when in move
	if (RollAngle != 0 && RollSpeed != 0) {
		FRotator ControlRotation = SurferCharacter->GetController()->GetControlRotation();
		ControlRotation.Roll = CameraBehaviour();
		SurferCharacter->GetController()->SetControlRotation(ControlRotation);
	}
	//Apply constatnt tick based braking/friction when on ground
	bFrameForBraking = IsMovingOnGround();
	//bCrouchFrameTolerated = IsCrouching();

}
