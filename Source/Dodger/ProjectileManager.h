
#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectileManager.generated.h"

/**
 * 
 */
UCLASS()
class DODGER_API UProjectileManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UProjectileManager* Get(const UObject* WorldContext);
	/**
	 *  Get a projectile from the pool (or spawn a new one if none available) and launch it
	 */
	AProjectile* LaunchProjectile(TSubclassOf<AProjectile> ProjectileClass, const FVector& Location, const FRotator& Rotation, APawn* Instigator);
	/**
	 *  Global delegate to detect any projectile hit
	 */
	FOnProjectileHitDelegate OnProjectileHitDelegate;
	
private:
	/**
	 *  Projectile hit detected 
	 */
	void OnProjectileHit(AProjectile* Projectile, const FHitResult& HitResult);
	/**
	 *  Return a projectile to the pool instead of destroying it
	 */
	void ReturnProjectileToPool(AProjectile* Projectile);
	
	// The pool of inactive projectiles
	UPROPERTY(Transient)
	TArray<TObjectPtr<AProjectile>> ProjectilePool;
};
