// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpeedGam340GameMode.h"
#include "SpeedGam340Character.h"
#include "UObject/ConstructorHelpers.h"

ASpeedGam340GameMode::ASpeedGam340GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
