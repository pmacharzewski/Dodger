#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatConfig.generated.h"

class AProjectile;
class UAnimMontage;

UCLASS()
class UCombatConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Class of the projectile to spawn when firing
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> ProjectileClass;
	/**
	 * Animation montage for the fire action
	 */
	UPROPERTY(EditAnywhere, Category = "Animations")
	TObjectPtr<UAnimMontage> AttackMontage;
	/**
	 * Animation montage for the dodge action
	 */
	UPROPERTY(EditAnywhere, Category = "Animations")
	TObjectPtr<UAnimMontage> DodgeMontage;
	/**
	 * Rate at which attacks can be performed (attacks per second)
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRate = 2.0f;
	/**
	 * Rate at which dodges can be performed (dodges per second)
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float DodgeRate = 2.0f;
	/**
	 * Offset for projectile aim origin.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AimOffset = 70.0f;
};