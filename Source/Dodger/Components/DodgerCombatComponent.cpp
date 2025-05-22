
#include "DodgerCombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Dodger/DodgerCharacter.h"
#include "Dodger/EnemyAIController.h"
#include "Dodger/HitValidationTypes.h"
#include "Dodger/Projectile.h"
#include "Dodger/ProjectileManager.h"
#include "Dodger/Data/CombatConfig.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(CombatLog, Log, All);

namespace
{
	constexpr TCHAR Label_SpawnProjectile[] = TEXT("SpawnProjectile");
	constexpr TCHAR Label_Invulnerable[] = TEXT("Invulnerable");
}

UDodgerCombatComponent::UDodgerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	if (!Config)
	{
		Config = GetDefault<UCombatConfig>();
	}
}

void UDodgerCombatComponent::PostInitProperties()
{
	Super::PostInitProperties();

	OwningCharacter = Cast<ADodgerCharacter>(GetOwner());
}

void UDodgerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!OwningCharacter.IsValid())
	{
		UE_LOG(CombatLog, Error, TEXT("[%hs] OwningCharacter is invalid."), __func__);
		return;
	}

	UAnimInstance* AnimInstance = OwningCharacter->GetMesh() ? OwningCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(CombatLog, Error, TEXT("[%hs] AnimInstance is invalid."), __func__);
		return;
	}
	
	// Bind animation notify delegates
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::OnMontageNotifyBegin);
	AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ThisClass::OnMontageNotifyEnd);
	AnimInstance->OnMontageStarted.AddDynamic(this, &ThisClass::OnMontageStart);
	AnimInstance->OnMontageEnded.AddDynamic(this, &ThisClass::OnMontageEnd);

	// Initialize late joining clients with current state
	if (GetWorld()->IsNetMode(NM_Client))
	{
		InitLateJoiners();
	}
}

void UDodgerCombatComponent::InitLateJoiners()
{
	// If there's an active montage, resume it at the correct position
	if (UAnimMontage* CurrentMontage = StateToMontage(CombatState))
	{
		const float PlayRate = CombatState == ECombatState::Attack ? Config->AttackRate : Config->DodgeRate;
		UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(CurrentMontage, PlayRate, EMontagePlayReturnType::MontageLength, MontagePlaybackPosition);
	}
	// Handle dead state for late joiners
	else if (CombatState == ECombatState::Dead)
	{
		OwningCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		OwningCharacter->GetMesh()->SetSimulatePhysics(true);
	}
}

void UDodgerCombatComponent::SetFireIntent(bool bActive)
{
	bFireIntent = bActive;
}

void UDodgerCombatComponent::SetDodgeIntent(bool bActive)
{
	bDodgeIntent = bActive;
}

void UDodgerCombatComponent::FireProjectile()
{
	// Only handle locally controlled characters to have correct Aim settings
	if (OwningCharacter->IsLocallyControlled())
	{
		// Calculate aim points
		FVector AimOrigin, AimTarget;
		EvaluateAimOriginAndTarget(AimOrigin, AimTarget);

		// Spawn projectile locally
		HandleSpawnProjectile(AimOrigin, AimTarget);

		// Notify simulated proxy to spawn locally
		if (OwningCharacter->HasAuthority())
		{
			NetMultiFireProjectile(AimOrigin, AimTarget);
		}
		else
		{
			ServerFireProjectile(AimOrigin, AimTarget);
		}
	}
}

void UDodgerCombatComponent::ServerDodge_Implementation(const FVector_NetQuantizeNormal& Direction)
{
	NetMultiDodge(Direction);
}

void UDodgerCombatComponent::NetMultiDodge_Implementation(const FVector_NetQuantizeNormal& Direction)
{
	// Skip if locally controlled (already handled)
	if (!OwningCharacter.IsValid() || OwningCharacter->IsLocallyControlled())
	{
		return;
	}

	// Update rotation and play dodge animation on simulated proxies
	OwningCharacter->SetActorRotation(Direction.Rotation());
	OwningCharacter->PlayAnimMontage(Config->DodgeMontage, Config->DodgeRate);
}

void UDodgerCombatComponent::ServerAttack_Implementation()
{
	NetMultiAttack();
}

void UDodgerCombatComponent::NetMultiAttack_Implementation()
{
	// Skip if locally controlled (already handled)
	if (!OwningCharacter.IsValid() || OwningCharacter->IsLocallyControlled())
	{
		return;
	}
	
	// Play attack animation on simulated proxies
	OwningCharacter->PlayAnimMontage(Config->AttackMontage, Config->AttackRate);
}

void UDodgerCombatComponent::UpdateCombat(float DeltaTime)
{
	// Only process if character isn't dead
	if (CombatState != ECombatState::Dead)
	{
		// Character wants to dodge
		if (bDodgeIntent)
		{
			if (CanDodge())
			{
				PerformDodge();
			}
		}
		// Character wants to fire - handled in second priority (attack can be interrupted)
		else if (bFireIntent)
		{
			if (CanAttack())
			{
				PerformAttack();
			}
		}
	}

	// Track current montage position for late joiners purposes
	if (!IsNetMode(NM_Client))
	{
		// replicate current montage position for late joiners
		if (UAnimMontage* CurrentMontage = StateToMontage(CombatState))
		{
			UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
			MontagePlaybackPosition = AnimInstance->Montage_GetPosition(CurrentMontage);
		}
	}

	// draw debug info
	if (!IsNetMode(NM_DedicatedServer))
	{
		static const TMap<ECombatState, FString> StateToStringMap = {
			{ECombatState::Idle, TEXT("Idle")},
			{ECombatState::Attack, TEXT("Attacking")},
			{ECombatState::Dodge, TEXT("Dodging")},
			{ECombatState::Dead, TEXT("Dead")}};
		
		DrawDebugString(GetWorld(), FVector(0,-20, 100),
			FString::Printf(TEXT("Health: %.1f \nState: %s \nInvulnerable: %s"),
				OwningCharacter->GetHealth(), *StateToStringMap[CombatState], bIsInvulnerable ? TEXT("true") : TEXT("false")),
			GetOwner(), FColor::White, 0, true);
	}
}

void UDodgerCombatComponent::PerformAttack()
{
	// Play animation locally
	if (OwningCharacter->IsLocallyControlled())
	{
		OwningCharacter->PlayAnimMontage(Config->AttackMontage, Config->AttackRate);
		
		if (OwningCharacter->HasAuthority())
		{
			NetMultiAttack();
		}
		else
		{
			ServerAttack();
		}
	}
}

bool UDodgerCombatComponent::CanAttack() const
{
	return Config->AttackMontage && !IsAttacking() && !IsDodging();
}

bool UDodgerCombatComponent::IsAttacking() const
{
	const UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
	return AnimInstance && AnimInstance->Montage_IsPlaying(Config->AttackMontage);
}

void UDodgerCombatComponent::PerformDodge()
{
	if (OwningCharacter->IsLocallyControlled())
	{
		const FVector Direction = ComputeDodgeDirection();
		
		// Cancel attack if currently attacking
		if (IsAttacking())
		{
			OwningCharacter->StopAnimMontage(Config->AttackMontage);
		}

		OwningCharacter->SetActorRotation(Direction.ToOrientationQuat());
		
		// Play dodge animation
		OwningCharacter->PlayAnimMontage(Config->DodgeMontage, Config->DodgeRate);
		
		if (OwningCharacter->HasAuthority())
		{
			NetMultiDodge(Direction);
		}
		else
		{
			ServerDodge(Direction);
		}
	}
}

bool UDodgerCombatComponent::CanDodge() const
{
	return Config->DodgeMontage && !IsDodging() && !OwningCharacter->GetCharacterMovement()->IsFalling();
}

bool UDodgerCombatComponent::IsDodging() const
{
	const UAnimInstance* AnimInstance = OwningCharacter->GetMesh()->GetAnimInstance();
	return AnimInstance && AnimInstance->Montage_IsPlaying(Config->DodgeMontage);
}

void UDodgerCombatComponent::ServerFireProjectile_Implementation(const FVector_NetQuantize& Origin, const FVector_NetQuantize& Target)
{
	NetMultiFireProjectile(Origin, Target);
}

void UDodgerCombatComponent::NetMultiFireProjectile_Implementation(const FVector_NetQuantize& Origin, const FVector_NetQuantize& Target)
{
	// local player already handled - handle simulated proxy and server only
	if (!OwningCharacter.IsValid() || OwningCharacter->IsLocallyControlled())
	{
		return;
	}
	
	HandleSpawnProjectile(Origin, Target);
}

void UDodgerCombatComponent::NetMultiServeFinalBlow_Implementation(const FVector_NetQuantizeNormal& Direction)
{
	CombatState = ECombatState::Dead;
	OwningCharacter->GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	OwningCharacter->GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	OwningCharacter->GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	OwningCharacter->GetMesh()->SetCollisionResponseToChannel(ECC_HitBox, ECR_Ignore);
	OwningCharacter->GetMesh()->SetSimulatePhysics(true);
	constexpr float LaunchSize = 8000.0f;
	OwningCharacter->GetMesh()->AddImpulse(Direction * LaunchSize, NAME_None, true);
}

void UDodgerCombatComponent::OnMontageStart(UAnimMontage* Montage)
{
	if (CombatState != ECombatState::Dead)
	{
		// Update combat state based on montage
		CombatState = MontageToState(Montage);
	}
}

void UDodgerCombatComponent::OnMontageEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (CombatState != ECombatState::Dead)
	{
		// interrupted means that another animation started which sets the state so don't override it
		if (!bInterrupted)
		{
			// ended animation without interruption - nothing happens - idle
			CombatState = ECombatState::Idle;
		}
	}
}

void UDodgerCombatComponent::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// Handle projectile spawn notify on attack montage
	if (NotifyName == Label_SpawnProjectile)
	{
		FireProjectile();
	}
	// Handle invulnerability start notify on dodge montage
	else if (NotifyName == Label_Invulnerable)
	{
		bIsInvulnerable = true;
	}
}

void UDodgerCombatComponent::OnMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// Disable invulnerability when dodge montage is close to finish (character is standing up)
	if (NotifyName == Label_Invulnerable)
	{
		bIsInvulnerable = false;
	}
}

FVector UDodgerCombatComponent::ComputeDodgeDirection() const
{
	if (AAIController* EnemyAI = Cast<AAIController>(OwningCharacter->GetController()))
	{
		// AI does random left/right dodge
		return FMath::FRand() > 0.5f ? OwningCharacter->GetActorRightVector() : -OwningCharacter->GetActorRightVector();
	}
	
	{ // player dodges in the input direction
		
		const auto Move = OwningCharacter->GetCharacterMovement();
		return Move->GetLastInputVector().IsNearlyZero()
			? OwningCharacter->GetActorForwardVector()
			: Move->GetLastInputVector().GetSafeNormal();
	}
}

void UDodgerCombatComponent::EvaluateAimOriginAndTarget(FVector& AimOrigin, FVector& AimTarget) const
{
	// Only handle locally controlled characters
	if (!OwningCharacter->IsLocallyControlled())
	{
		return;
	}

	// Calculate base aim origin (where projectile starts)
	AimOrigin = OwningCharacter->GetPawnViewLocation() + OwningCharacter->GetActorForwardVector() * Config->AimOffset;

	// AI-specific targeting
	if (AEnemyAIController* EnemyAI = Cast<AEnemyAIController>(OwningCharacter->GetController()))
	{
		AimTarget = EnemyAI->GetTargetActor() ? EnemyAI->GetTargetActor()->GetActorLocation() : AimOrigin + EnemyAI->GetPawn()->GetActorForwardVector() * Config->AimOffset;
		AimTarget += FMath::VRandCone(OwningCharacter->GetActorForwardVector(), 30.0f) * 60; 
	}
	else
	{
		// Player-specific targeting based on camera direction
		constexpr float FarAway = 1000000.0f;
		const FVector CamLoc = OwningCharacter->GetFollowCamera()->GetComponentLocation();
		const FVector CamDir = OwningCharacter->GetFollowCamera()->GetForwardVector();
		const FVector TraceStart = CamLoc;
		const FVector TraceEnd = TraceStart + CamDir * FarAway;

		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
	
		AimTarget = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;
	}
}

void UDodgerCombatComponent::HandleSpawnProjectile(const FVector& AimOrigin, const FVector& AimTarget)
{
	FVector Dir = (AimTarget - AimOrigin).GetSafeNormal();
	UProjectileManager::Get(this)->LaunchProjectile(Config->ProjectileClass, AimOrigin, Dir.Rotation(), Cast<APawn>(GetOwner()));
}

ECombatState UDodgerCombatComponent::MontageToState(UAnimMontage* Montage) const
{
	if (Montage == Config->AttackMontage) return ECombatState::Attack;
	if (Montage == Config->DodgeMontage)  return ECombatState::Dodge;
	
	return ECombatState::Idle;
}

UAnimMontage* UDodgerCombatComponent::StateToMontage(ECombatState State) const
{
	switch (State)
	{
	case ECombatState::Attack: return Config->AttackMontage;
	case ECombatState::Dodge:  return Config->DodgeMontage;
	default: return nullptr;
	}
}

void UDodgerCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Only send initial state to clients for late joiners
	DOREPLIFETIME_CONDITION(UDodgerCombatComponent, CombatState, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(UDodgerCombatComponent, MontagePlaybackPosition, COND_InitialOnly);
}
