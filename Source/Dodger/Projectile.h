// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class ADodgerCharacter;
class UProjectileConfig;
class USphereComponent;
class UProjectileMovementComponent;

using FOnProjectileHitDelegate = TMulticastDelegate<void(AProjectile* Projectile, const FHitResult& ImpactResult)>;

UCLASS()
class DODGER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	void Activate();
	
	void Deactivate();
	
	bool IsActive() const { return bIsActive; }
	
	FOnProjectileHitDelegate OnProjectileHitDelegate;
	
protected:
	// Start Base Interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	// End Base Interface
	
	virtual void HandleCharacterHit(ADodgerCharacter* Attacker, ADodgerCharacter* HitCharacter, const FHitResult& ImpactResult);
	virtual void PlayHitEffects();
	
private:
	
	UFUNCTION()
	void OnProjectileHit(const FHitResult& ImpactResult);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	
	FVector StartLocation = FVector::ZeroVector;
	FVector InitialVelocity = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, NoClear)
	TObjectPtr<const UProjectileConfig> Config;

	bool bIsActive = false;
	
};
