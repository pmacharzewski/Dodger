
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dodger/HitValidationTypes.h"
#include "HitValidationComponent.generated.h"

class ADodgerCharacter;

UCLASS(Within=DodgerCharacter)
class UHitValidationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitValidationComponent();
	/**
	 * Server RPC to reconcile projectile hits with server-side rewind.
	 */
	UFUNCTION(Server, Reliable)
	void ServerReconcileProjectileHit(ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);
protected:
	// Base Interface Start
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// Base Interface End

private:
	// Verify hit with server-side rewind
	FHitVerificationResult VerifyProjectileHit(ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime) const;
	
	// Frame history management
	void SaveCurrentFrame();
	void CaptureCharacterFrame(FCharacterFrameData& OutFrameData);
	
	// Rewind functionality
	FCharacterFrameData FindRewindFrame(ADodgerCharacter* TargetCharacter, float HitTime) const;
	FCharacterFrameData InterpolateFrames(const FCharacterFrameData& OlderFrame, const FCharacterFrameData& YoungerFrame, float HitTime) const;

	// Hitbox manipulation
	void CaptureHitboxPositions(ADodgerCharacter* TargetCharacter, FCharacterFrameData& OutFrameData) const;
	void ApplyFrameToCharacter(ADodgerCharacter* TargetCharacter, const FCharacterFrameData& FrameData) const;
	void RestoreCharacterHitboxes(ADodgerCharacter* TargetCharacter, const FCharacterFrameData& FrameData) const;

	// Hit verification
	FHitVerificationResult CheckProjectileCollision(const FCharacterFrameData& FrameData, ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime) const;
	
private:
	
	UPROPERTY(EditAnywhere)
	int32 MaxFrameHistory = 240; // ~4 seconds at 60fps

	TUniquePtr<TCircularBuffer<FCharacterFrameData>> FrameHistory;
	uint32 FrameCounter = 0;
	
	UPROPERTY(Transient)
	TObjectPtr<ADodgerCharacter> OwnerCharacter = nullptr;
};
