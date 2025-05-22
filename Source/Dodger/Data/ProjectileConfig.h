#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectileConfig.generated.h"

class UNiagaraSystem;
class USoundCue;

UCLASS()
class UProjectileConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Speed of the projectile
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float Speed = 3000.0f;
	/**
	 * Gravity scale applied to the projectile's movement
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float GravityScale = 1.0f;
	/**
	 * Radius of the projectile's collision sphere
	 */
	UPROPERTY(EditAnywhere, Category = "Collision")
	float Radius = 14.0f;
	/**
	 * Damage dealt by the projectile on impact
	 */
	UPROPERTY(EditAnywhere, Category = "Damage")
	float Damage = 20.0f;
	/**
	 * Sound played when the projectile hits a target
	 */
	UPROPERTY(EditAnywhere, Category = "Effects")
	TObjectPtr<USoundCue> HitSound;
	/**
	 * Niagara system effect played on impact
	 */
	UPROPERTY(EditAnywhere, Category = "Effects")
	TObjectPtr<UNiagaraSystem> HitEffect;
};