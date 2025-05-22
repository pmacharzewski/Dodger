
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DodgerCombatComponent.generated.h"

class UBoxComponent;
class AProjectile;
class ADodgerCharacter;
class UCombatConfig;

UENUM()
enum class ECombatState : uint8
{
	Idle,
	Attack,
	Dodge,
	Dead
};

UCLASS()
class DODGER_API UDodgerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UDodgerCombatComponent();

	// Set the character's intent to fire/attack
	UFUNCTION(BlueprintCallable)
	void SetFireIntent(bool bActive);

	// Set the character's intent to dodge
	UFUNCTION(BlueprintCallable)
	void SetDodgeIntent(bool bActive);

	// Getters for combat state and status
	ECombatState GetState() const { return CombatState; };
	bool IsInvulnerable() const { return bIsInvulnerable; }
	bool IsDodging() const;
	bool IsAttacking() const;
	
	// Main update function called every frame
	void UpdateCombat(float DeltaTime);
	
	// Handle final attack when character health drops to 0
	UFUNCTION(NetMulticast, Reliable)
	void NetMultiServeFinalBlow(const FVector_NetQuantizeNormal& Direction);
protected:
	// Initialize late joining players with current state
	void InitLateJoiners();
	
	// Base Interface
	virtual void PostInitProperties() override;
	virtual void BeginPlay() override;
	
	// Fire Logic
	void PerformAttack();
	bool CanAttack() const;
	void FireProjectile();
	
	// Dodge Logic
	void PerformDodge();
	bool CanDodge() const;
	
private:
	
	// RPCs for attack montage
	UFUNCTION(Server, Reliable)
	void ServerAttack();
	UFUNCTION(NetMulticast, Unreliable)
	void NetMultiAttack();

	// RPCs for spawning projectile
	UFUNCTION(Server, Reliable)
	void ServerFireProjectile(const FVector_NetQuantize& Origin, const FVector_NetQuantize& Target);
	UFUNCTION(NetMulticast, Unreliable)
	void NetMultiFireProjectile(const FVector_NetQuantize& Origin, const FVector_NetQuantize& Target);

	// RPCs for dodge action
	UFUNCTION(Server, Reliable)
	void ServerDodge(const FVector_NetQuantizeNormal& Direction);
	UFUNCTION(NetMulticast, Unreliable)
	void NetMultiDodge(const FVector_NetQuantizeNormal& Direction);
	
	// Montages notifiers
	UFUNCTION()
	void OnMontageStart(UAnimMontage* Montage);
	UFUNCTION()
	void OnMontageEnd(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);
	UFUNCTION()
	void OnMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);
	
	// Helpers
	FVector ComputeDodgeDirection() const;
	void EvaluateAimOriginAndTarget(FVector& AimOrigin, FVector& AimTarget) const;
	void HandleSpawnProjectile(const FVector& AimOrigin, const FVector& AimTarget);
	ECombatState MontageToState(UAnimMontage* Montage) const;
	UAnimMontage* StateToMontage(ECombatState State) const;
	
	// Combat Config
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<const UCombatConfig> Config;

	// Reference to owning character
	TWeakObjectPtr<ADodgerCharacter> OwningCharacter;
	
	// Fire Dodge Intents
	bool bFireIntent = false;
	bool bDodgeIntent = false;

	// Invulnerability flag set during dodge action
	bool bIsInvulnerable = false;

	// Replicated properties for late joiners
	UPROPERTY(Replicated)
	ECombatState CombatState = ECombatState::Idle;
	UPROPERTY(Replicated)
	float MontagePlaybackPosition = 0.0f;
};