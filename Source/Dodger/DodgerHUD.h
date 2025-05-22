
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DodgerHUD.generated.h"

/**
 * 
 */
UCLASS()
class DODGER_API ADodgerHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void DrawHUD() override;
};
