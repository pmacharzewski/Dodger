
#include "DodgerAnimInstance.h"

#include "DodgerCharacter.h"

void UDodgerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Dodger = Cast<ADodgerCharacter>(TryGetPawnOwner());
}

void UDodgerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!Dodger)
	{
		return;
	}

	bIsDodging = Dodger->IsDodging();
}
