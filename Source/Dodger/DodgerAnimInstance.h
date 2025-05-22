
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DodgerAnimInstance.generated.h"

class ADodgerCharacter;
/**
 * 
 */
UCLASS()
class DODGER_API UDodgerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsDodging = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ADodgerCharacter> Dodger;
};
