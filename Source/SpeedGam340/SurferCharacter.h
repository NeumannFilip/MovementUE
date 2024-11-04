 // Fill out your copyright notice in the Description page of Project Settings.


//folder speeed z plz work
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Runtime/Launch/Resources/Version.h"

#include "SurferCharacter.generated.h"

/* Surfer Character will contain entire values and function that handle the behavior
* I want to replicate the Half-Life or Counter Strike movement. Especially in terms of
* Bunnyhopping or strafing and ideally Surfing - meaning that character can move on angled
* surfaces and gain velocity.
*
* Sources I took inspirations and knowledge of.
* https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/GameFramework/ACharacter/
* https://docs.unrealengine.com/5.1/en-US/movement-components-in-unreal-engine/
* https://projectborealis.com/
* http://adrianb.io/2015/02/14/bunnyhop.html
* 
* documentation of the fucnctions
* 
* https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/GameplayArchitecture/Functions/
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/ACharacter/CanJumpInternal_Implementation/
* https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/ACharacter/OnJumped_Implementation/
* https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/GameFramework/ACharacter/ClearJumpInput/
* https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/GameFramework/ACharacter/OnMovementModeChanged/
* https://www.google.com/search?client=opera-gx&q=K2_OnMovementModeChanged&sourceid=opera&ie=UTF-8&oe=UTF-8
*/


class USurferMovementComponent;



UCLASS(config = Game)
class SPEEDGAM340_API ASurferCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:
	//As I need to use custom movement component I have override the character properties
	// Sets default values for this character's properties
	ASurferCharacter(const FObjectInitializer& ObjectInitializer);


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//Jump functions to override
	//Update jump input state after having checked input.
	virtual void ClearJumpInput(float DeltaTime) override;//
	//Fire off jumping
	void Jump() override;//
	//Stop Jump
	virtual void StopJumping() override;//
	//Event fired when the character has just started jumping
	virtual void OnJumped_Implementation() override;//
	//Customizable event to check if the character can jump in the current state. Default implementation returns true if the character is on the ground and not crouching
	virtual bool CanJumpInternal_Implementation() const override;//

	//here was supposed to be crunch functions but deleted them since they were extra buggy
	//void RecalculateBaseEyeHeight() override;//

	//MovementMode handler that adjust physics on player depending on type of movement hes in: i.e if on ground apply gravity friction etc.
	void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;//

	//Tracking jump time to avoid glitches 
	float GetLastJumpTime() {//
		return TrackLastTimeJump;
	}


	//Modificators to the player
#pragma region Modificators to the movement 

	//From what I read  I can  change values into functions and make them blueprintable.
	//Also at the same time It is possible to assign function in the functions
	//I will use this to let other function use the modified values
	//https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/GameplayArchitecture/Functions/

	//returning current turn rate
	UFUNCTION(Category = "Get Surfer Stats", BlueprintPure) FORCEINLINE float GetBaseTurnRate() const//
	{
		return BaseTurnRate; // X axis
	};
	//Setting current turn rate
	UFUNCTION(Category = "Set Surfer Stats", BlueprintCallable) void SetBaseTurnRate(float val)//
	{
		BaseTurnRate = val;
	};
	//returning current look up rate
	UFUNCTION(Category = "Get Surfer Stats", BlueprintPure) FORCEINLINE float GetBaseLookUpRate() const//
	{
		return BaseLookUpRate;
	};
	//setting current look up rate
	UFUNCTION(Category = "Set Surfer Stats", BlueprintCallable) void SetBaseLookUpRate(float val)//
	{
		BaseLookUpRate = val;
		
	};
	//for autobunnyhop that will elimate need skill aspect
	//forceinline to make VS compile it first
	UFUNCTION(Category = "Get Surfer Stats", BlueprintPure) FORCEINLINE bool GetAutoBunnyhop() const//
	{
		return bAutoBunnyhop;
	};
	//set the val of bhop
	UFUNCTION(Category = "Set Surfer Stats", BlueprintCallable) void SetAutoBunnyhop(bool val)//
	{
		bAutoBunnyhop = val;
	};
	//pointer to movement to keep track 
	UFUNCTION(Category = "Get Surfer Stats", BlueprintPure) FORCEINLINE USurferMovementComponent* GetMovementPtr() const//
	{
		return MovementPointer;
	};


	UFUNCTION()
		bool IsSprinting() const//
	{
		return bIsSprinting;
	}
	UFUNCTION()
		bool DoesWantToWalk() const//
	{
		return bWantsToWalk;
	}

	float GetDefaultBaseEyeHeight() const { return DefaultBaseEyeHeight; }//

	UFUNCTION()
		void ToggleNoClip();//

	UFUNCTION(BlueprintPure, Category = "Movement")
		float GetMinSpeedForFallDamage() const { return MinSpeedForFallDamage; }

	float GetMinLandBounceSpeed() const { return LandJumpSpeed; }
#pragma endregion Modificators

	//These function will hold the logic for executing movement
	UFUNCTION()
		void Move(FVector Direction, float Value);// move 


	UFUNCTION()
		void Turn(bool bIsPure, float Rate);// X axis

	UFUNCTION()
		void LookUp(bool bIsPure, float Rate);// Y axis

	//good morning i dont understand custom crouch functions so i got rid of them
	//virtual bool CanCrouch() const override;//
private:

	//For crouching camera adjustment
		float DefaultBaseEyeHeight;//

		//Tracking jump time to avoid glitches and put cooldown
		float TrackLastTimeJump;//

		//irrelevant
		//float LastJumpBoostTime;//

		//max time in jump
		float MaxJumpTime;//

		
		//properties

	//Since the movement is depended on camera and mouse, I need to set up movement accordingly

		UPROPERTY(VisibleAnywhere, Category = "Surfing",meta = (AllowPrivateAccess = "true"))//
			float BaseLookUpRate; // Y axis

		UPROPERTY(VisibleAnywhere, Category = "Surfing", meta = (AllowPrivateAccess = "true"))//
			float BaseTurnRate;// X axis 

		UPROPERTY(EditAnywhere, Category = "Surfing", meta = (AllowPrivateAccess = "true"))//
			bool bAutoBunnyhop; // potential triggering auto bhop = meaning that just hold space to keep jumping

		UPROPERTY(EditDefaultsOnly, Category = "Surfing", meta = (AllowPrivateAccess = "true"))//
			float LandJumpSpeed;

		UPROPERTY(EditDefaultsOnly, Category = "Surfing", meta = (AllowPrivateAccess = "true"))
			float MinSpeedForFallDamage;

		//Thats probably needed in boosted jumps which i dont wantr
		UPROPERTY(EditDefaultsOnly, Category = "Surfing", meta = (AllowPrivateAccess = "true"))//
			float CapDamageMomentumZ = 0.f;

		//pointing to movement component
		USurferMovementComponent* MovementPointer;//

		//checking for modes
		bool bIsSprinting;//

		bool bWantsToWalk;//

		//
		bool bDeferJumpStop;//

		//virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInsitgator, AActor* DamageCauser) override;//


};

//Spline for normalizing curve.
//Inline allows to work indepenedly and refernce by rest of the code
inline float Spline(float Value) {
	const float SquaringValue = Value * Value;
	return (3.0f * SquaringValue - 2.0f * SquaringValue * Value);
}
