// Fill out your copyright notice in the Description page of Project Settings.


#include "SurferCharacter.h"


#include "SurferMovementComponent.h"

#include "Components/CapsuleComponent.h"

#include "HAL/IConsoleManager.h"


//Small little console that I set up for a small cheating and extra options press '`' to open
static TAutoConsoleVariable<int32> CVarBunnyhop(TEXT("move.Bunnyhopping"), 0, TEXT("BHOP ON\n"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAutoBHop(TEXT("move.Jumping"), 1, TEXT("Holding space makes player jump\n"), ECVF_Default);



//This cpp file contains logic for handling movement.
//It is based on capsule component that will interact with world and objects.
//

//Sources


// Sets default values
//Changing constructor to override ObjectInitializer with custom movement component.
ASurferCharacter::ASurferCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USurferMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Instantiaitng CapsuleComponent / interaction object
	GetCapsuleComponent()->InitCapsuleSize(30.48f, 68.50f);
	//Setting collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

	//Setting values for camera movement
	BaseTurnRate = 45.f; //X axis

	BaseLookUpRate = 45.f;// Y axis

	//for crouching but dropped this 
	DefaultBaseEyeHeight = 54.f;
	BaseEyeHeight = DefaultBaseEyeHeight;
	const float CrouchedHalfHeight = 68.50f / 2.0f;
	CrouchedEyeHeight = 54.f - CrouchedHalfHeight;

	LandJumpSpeed = 330.f;

	//pointer that casts movement component to surfer
	MovementPointer = Cast<USurferMovementComponent>(ACharacter::GetMovementComponent());

	//CapDamageMomentumZ = 475.f;

}

//No noclip for now
void ASurferCharacter::ToggleNoClip()
{
} 


void ASurferCharacter::Move(FVector Direction, float Value)
{
	if (!FMath::IsNearlyZero(Value)) {

		AddMovementInput(Direction, Value);
	}
}

void ASurferCharacter::Turn(bool bIsPure, float Rate)
{
	if (!bIsPure) {
		Rate = Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds();
	}

	AddControllerYawInput(Rate);
}

void ASurferCharacter::LookUp(bool bIsPure, float Rate)
{
	if (!bIsPure) {
		Rate = Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds();
	}
}

/*bool ASurferCharacter::CanCrouch() const
{
	return !GetCharacterMovement()->bCheatFlying && Super::CanCrouch() && !MovementPtr->IsOnLadder();
}*/

// Called when the game starts or when spawned
void ASurferCharacter::BeginPlay()
{
	Super::BeginPlay();
	MaxJumpTime = -4.0f * GetCharacterMovement()->JumpZVelocity / (3.0f * GetCharacterMovement()->GetGravityZ());
}

// Called every frame
void ASurferCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//Whenever player exceeds time on the surface call stop jump function;
	if (bDeferJumpStop) {
		bDeferJumpStop = false;
		Super::StopJumping();
	}

}

// Called to bind functionality to input
void ASurferCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

/// <summary>
/// Keep Clearing jumping if cheats are turned on.
/// </summary>
/// <param name="DeltaTime"></param>
void ASurferCharacter::ClearJumpInput(float DeltaTime)
{
	//In case of turned on bhop keep cleared the queue and just keep jumping
	if (CVarAutoBHop.GetValueOnGameThread() != 0 || bAutoBunnyhop || GetCharacterMovement()->bCheatFlying || bDeferJumpStop)
	{
		return;
	}
	Super::ClearJumpInput(DeltaTime);
}

/// <summary>
/// This handles the option for jumping. 
/// It check if player cannot jump if its in air
/// </summary>
void ASurferCharacter::Jump()
{
	//Checking if the players is in air 
	//if true check for ground interaction
	if (GetCharacterMovement()->IsFalling()) {
		bDeferJumpStop = true;
	}

	Super::Jump();
}

/// <summary>
/// If jumping is not "put off" stop jumping.
/// </summary>
void ASurferCharacter::StopJumping()
{
	if (!bDeferJumpStop)
	{
		Super::StopJumping();
	}
}

void ASurferCharacter::OnJumped_Implementation()
{
	//there was meant to be logic for custom jumping method that is unncesesary 
}

/// <summary>
///  This handles checks for the player and allow him to jump if its possible  in the state he is in
/// </summary>
/// <returns></returns>
bool ASurferCharacter::CanJumpInternal_Implementation() const
{
	
	//assign bool to character movmeent and make it pass to jump.
	bool bCanJumpInState = GetCharacterMovement() && GetCharacterMovement()->IsJumpAllowed();

	//If player can jump check for possibilities
	if (bCanJumpInState) {

		//If he didnt jump or jump hold wasn't fired check for the jumps made
		if (!bWasJumping || GetJumpMaxHoldTime() <= 0.f) {
			//Add jumps to the state.
			if (JumpCurrentCount == 0 && GetCharacterMovement()->IsFalling()) {
				bCanJumpInState = JumpCurrentCount + 1 < JumpMaxCount;
			}
			//else just maintain it, to current be less than max
			else {
				bCanJumpInState = JumpCurrentCount < JumpMaxCount;
			}
		}
		else {
			//detect if jump is pressed 
			const bool bJumpKeyHeld = (bPressedJump && JumpKeyHoldTime < GetJumpMaxHoldTime());
			//assign jump and decide that character is on ground
			bCanJumpInState = bJumpKeyHeld && (GetCharacterMovement()->IsMovingOnGround() || (JumpCurrentCount < JumpMaxCount) || (bWasJumping && JumpCurrentCount == JumpMaxCount));

		}
		//Define what is floor and allow character to jump on floor
		if (GetCharacterMovement()->IsMovingOnGround()) {
			float FloorZ = FVector(0.0f, 0.0f, 1.0f) | GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
			float WalkableFloor = GetCharacterMovement()->GetWalkableFloorZ();
			bCanJumpInState &= (FloorZ >= WalkableFloor || FMath::IsNearlyEqual(FloorZ, WalkableFloor));
		}
	}
	
	return bCanJumpInState;

}


/// <summary>
/// //This is a function that serves as a simple checker and tracker of current mode of movement
	//Especially Jump and Falling as these do contain different logic of air movement than ground walk
/// </summary>
/// <param name="PrevMovementMode"></param>
/// <param name="PrevCustomMode"></param>
void ASurferCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
	//jump check
	if (!bPressedJump) {
		ResetJumpState();
	}
	////Check for falling(which is after jump force finished
	if (GetCharacterMovement()->IsFalling()) {
		if (bProxyIsJumpForceApplied) {
			ProxyJumpForceStartedTime = GetWorld()->GetTimeSeconds();
		}
	}
	else {
		////Reset values if not
		JumpCurrentCount = 0;
		JumpKeyHoldTime = 0.0f;
		JumpForceTimeRemaining = 0.0f;
		bWasJumping = false;


	}
	//https://www.google.com/search?client=opera-gx&q=K2_OnMovementModeChanged&sourceid=opera&ie=UTF-8&oe=UTF-8
	//Notifying the character that mode has changed
	//Its essentially delegate.
	K2_OnMovementModeChanged(PrevMovementMode, GetCharacterMovement()->MovementMode, PrevCustomMode, GetCharacterMovement()->CustomMovementMode);
	MovementModeChangedDelegate.Broadcast(this, PrevMovementMode, PrevCustomMode);
}

