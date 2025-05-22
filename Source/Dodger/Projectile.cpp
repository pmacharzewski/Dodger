// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "DodgerCharacter.h"
#include "DodgerPlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/DodgerCombatComponent.h"
#include "Components/HitValidationComponent.h"
#include "Components/SphereComponent.h"
#include "Data/ProjectileConfig.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

DEFINE_LOG_CATEGORY_STATIC(ProjectileLog, Log, All);

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	if (!Config)
	{
		Config = GetDefault<UProjectileConfig>();
	}
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetSphereRadius(Config ? Config->Radius : 14.0f);
	
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	
	ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &ThisClass::OnProjectileHit);
	ProjectileMovementComponent->InitialSpeed = Config ? Config->Speed : 3000.0f;
	ProjectileMovementComponent->MaxSpeed = Config ? Config->Speed : 3000.0f;
	ProjectileMovementComponent->ProjectileGravityScale = Config ? Config->GravityScale : 1.0f;
	ProjectileMovementComponent->bAutoActivate = false;
}

void AProjectile::Activate()
{
	SetActorTickEnabled(true);
	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	ProjectileMovementComponent->SetComponentTickEnabled(true);
	ProjectileMovementComponent->Velocity = GetActorForwardVector() * Config->Speed;
	ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
	ProjectileMovementComponent->UpdateComponentVelocity();
	ProjectileMovementComponent->SetActive(true, true);
	
}

void AProjectile::Deactivate()
{
	bIsActive = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	// Stop movement
	ProjectileMovementComponent->StopMovementImmediately();
	ProjectileMovementComponent->Deactivate();
}

void AProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (Config)
	{
		ProjectileMovementComponent->InitialSpeed = Config->Speed;
		ProjectileMovementComponent->MaxSpeed = Config->Speed;
		ProjectileMovementComponent->ProjectileGravityScale = Config->GravityScale;
	
		CollisionComponent->SetSphereRadius(Config->Radius);
		//ProjectileMesh->SetRelativeScale3D()
	}
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!Config)
	{
		UE_LOG(ProjectileLog, Error, TEXT("Projectile %s: Config is not set."), *GetName());
		Destroy();
		return;
	}
	
	StartLocation = GetActorLocation();
	InitialVelocity = ProjectileMovementComponent->Velocity;
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::HandleCharacterHit(ADodgerCharacter* Attacker, ADodgerCharacter* HitCharacter, const FHitResult& ImpactResult)
{
	if (Attacker == HitCharacter)
	{
		return;
	}
	
	if (Attacker->IsLocallyControlled())
	{
		// apply damage with server side rewind for local client
		if (Attacker->IsNetMode(NM_Client))
		{
			const ADodgerPlayerController* PlayerController = Cast<ADodgerPlayerController>(Attacker->GetController());
			Attacker->GetHitValidation()->ServerReconcileProjectileHit(HitCharacter, StartLocation, InitialVelocity, PlayerController->GetServerTime());
		}
		else
		{
			// server side - do instant check
			if (!HitCharacter->IsInvulnerable())
			{
				float DamageMultiplier = (Cast<UBoxComponent>(ImpactResult.GetComponent()) == HitCharacter->GetHitBoxHead()) ? 2.0f : 1.0f;
				UGameplayStatics::ApplyDamage(HitCharacter, Config->Damage * DamageMultiplier, Attacker->Controller, Attacker, UDamageType::StaticClass());
			}
		}
	}
}

void AProjectile::PlayHitEffects()
{
	if (Config->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Config->HitSound, GetActorLocation());
	}

	if (Config->HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Config->HitEffect, GetActorLocation(), GetActorRotation());
	}
}

void AProjectile::OnProjectileHit(const FHitResult& ImpactResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Stop Movement");
	
	if (!GetInstigator())
	{
		UE_LOG(ProjectileLog, Warning, TEXT("Projectile %s: No instigator set."), *GetName());
		Destroy();
		return;
	}

	ADodgerCharacter* InstigatorCharacter = Cast<ADodgerCharacter>(GetInstigator());
	
	if (ADodgerCharacter* HitCharacter = Cast<ADodgerCharacter>(ImpactResult.GetActor()))
	{
		HandleCharacterHit(InstigatorCharacter, HitCharacter, ImpactResult);
	}

	if (InstigatorCharacter && InstigatorCharacter->HasAuthority())
	{
		InstigatorCharacter->MakeNoise(1, InstigatorCharacter, GetActorLocation());
	}
	
	PlayHitEffects();

	OnProjectileHitDelegate.Broadcast(this, ImpactResult);
	
	//Destroy();
}

