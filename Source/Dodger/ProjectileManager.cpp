

#include "ProjectileManager.h"

UProjectileManager* UProjectileManager::Get(const UObject* WorldContext)
{
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetSubsystem<UProjectileManager>();
	}

	return nullptr;
}

AProjectile* UProjectileManager::LaunchProjectile(TSubclassOf<AProjectile> ProjectileClass, const FVector& Location, const FRotator& Rotation, APawn* Instigator)
{
	for (int32 i = 0 ; i < ProjectilePool.Num(); ++i)
	{
		AProjectile* Projectile = ProjectilePool[i];
		if (Projectile && !Projectile->IsActive() && Projectile->IsA(ProjectileClass))
		{
			// Remove from pool before reactivating
			ProjectilePool.RemoveAtSwap(i, EAllowShrinking::No);
            
			// Reactivate the projectile
			Projectile->SetActorLocationAndRotation(Location, Rotation);
			Projectile->SetInstigator(Instigator);
			Projectile->Activate();
			return Projectile;
		}
	}

	// If no projectile is available, spawn a new one
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Instigator;
	AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, Location, Rotation, SpawnParams);
	NewProjectile->Activate();
	if (NewProjectile)
	{
		NewProjectile->OnProjectileHitDelegate.AddUObject(this, &UProjectileManager::OnProjectileHit);
	}

	return NewProjectile;
}

void UProjectileManager::ReturnProjectileToPool(AProjectile* Projectile)
{
	Projectile->Deactivate();
	ProjectilePool.Add(Projectile);
}

void UProjectileManager::OnProjectileHit(AProjectile* Projectile, const FHitResult& HitResult)
{
	OnProjectileHitDelegate.Broadcast(Projectile, HitResult);
	
	ReturnProjectileToPool(Projectile);
}
