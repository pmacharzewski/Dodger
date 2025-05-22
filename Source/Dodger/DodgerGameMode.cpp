// Copyright Epic Games, Inc. All Rights Reserved.

#include "DodgerGameMode.h"
#include "DodgerCharacter.h"
#include "DodgerHUD.h"
#include "DodgerPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ADodgerGameMode::ADodgerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/BP_DodgerCharacter"));
	
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
	PlayerControllerClass = ADodgerPlayerController::StaticClass();
	HUDClass = ADodgerHUD::StaticClass();
}
