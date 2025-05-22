

#include "HitValidationComponent.h"

#include "Components/BoxComponent.h"
#include "Dodger/DodgerCharacter.h"
#include "Dodger/Data/ProjectileConfig.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"

DEFINE_LOG_CATEGORY_STATIC(HitValidationLog, Log, All);

UHitValidationComponent::UHitValidationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 1.0f / 60.0f;
}

void UHitValidationComponent::ServerReconcileProjectileHit_Implementation(ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (!TargetCharacter)
	{
		return;
	}
	
	FHitVerificationResult Confirm = TargetCharacter->GetHitValidation()->VerifyProjectileHit(TargetCharacter, TraceStart, InitialVelocity, HitTime);

	if (Confirm.bIsValidHit)
	{
		const float Damage = GetDefault<UProjectileConfig>()->Damage * (Confirm.bIsHeadshot ? 2.0f : 1.0f);
		AController* Controller = Cast<APawn>(GetOwner())->GetController();
		UGameplayStatics::ApplyDamage(TargetCharacter, Damage, Controller, GetOwner(), UDamageType::StaticClass());
	}
}

void UHitValidationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->IsNetMode(NM_ListenServer) || GetWorld()->IsNetMode(NM_DedicatedServer))
	{
		OwnerCharacter = Cast<ADodgerCharacter>(GetOwner());
		FrameHistory = MakeUnique<TCircularBuffer<FCharacterFrameData>>(MaxFrameHistory);
		SetComponentTickEnabled(true);
	}
}

void UHitValidationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// this runs on server only - character is cached server side
	if (OwnerCharacter)
	{
		SaveCurrentFrame();
	}
}

FHitVerificationResult UHitValidationComponent::VerifyProjectileHit(ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart,const FVector_NetQuantize100& InitialVelocity, float HitTime) const
{
	if (TargetCharacter)
	{
		const FCharacterFrameData FrameToCheck = FindRewindFrame(TargetCharacter, HitTime);
		return CheckProjectileCollision(FrameToCheck, TargetCharacter, TraceStart, InitialVelocity, HitTime);
	}
	
	constexpr FHitVerificationResult InvalidResult{false, false};
	return InvalidResult;
}

void UHitValidationComponent::SaveCurrentFrame()
{
	FCharacterFrameData CurrentFrame;
	CaptureCharacterFrame(CurrentFrame);
	(*FrameHistory)[FrameCounter++] = MoveTemp(CurrentFrame);
}

void UHitValidationComponent::CaptureCharacterFrame(FCharacterFrameData& OutFrameData)
{
	OutFrameData.Timestamp = GetWorld()->GetTimeSeconds();
	OutFrameData.Character = OwnerCharacter;
	OutFrameData.bIsInvulnerable = OwnerCharacter->IsInvulnerable();
	CaptureHitboxPositions(OwnerCharacter, OutFrameData);
}

FCharacterFrameData UHitValidationComponent::FindRewindFrame(ADodgerCharacter* TargetCharacter, float HitTime) const
{
	static const FCharacterFrameData InvalidFrame;

	// Early out if we have no frame history
	if (FrameCounter < 1)
	{
		return InvalidFrame;
	}

	// Start search from the newest frame (most recent)
	uint32 CurrentIndex = FrameCounter - 1;
	uint32 OlderIndex = CurrentIndex;

	// Walk backward through history to find the first frame older than the hit time
	while (OlderIndex > 0 && (*FrameHistory)[OlderIndex].Timestamp > HitTime)
	{
		CurrentIndex = OlderIndex;
		OlderIndex--;
	}

	// Found an exact timestamp match (rare but possible)
	if (FMath::IsNearlyEqual((*FrameHistory)[OlderIndex].Timestamp, HitTime))
	{
		return (*FrameHistory)[OlderIndex];
	}
	// Need interpolation
	if (OlderIndex != CurrentIndex)
	{
		return InterpolateFrames((*FrameHistory)[OlderIndex], (*FrameHistory)[CurrentIndex], HitTime);
	}
	
	// HitTime is newer than our newest frame
	if (HitTime >= (*FrameHistory)[CurrentIndex].Timestamp)
	{
		return (*FrameHistory)[CurrentIndex];
	}

	return InvalidFrame;
}

FCharacterFrameData UHitValidationComponent::InterpolateFrames(const FCharacterFrameData& OlderFrame, const FCharacterFrameData& YoungerFrame, float HitTime) const
{
	const float FrameInterval = YoungerFrame.Timestamp - OlderFrame.Timestamp;
	const float InterpAlpha = FMath::Clamp((HitTime - OlderFrame.Timestamp) / FrameInterval, 0.0f, 1.0f);

	FCharacterFrameData InterpolatedFrame;
	InterpolatedFrame.Timestamp = HitTime;
	InterpolatedFrame.bIsInvulnerable = FMath::Abs(HitTime - OlderFrame.Timestamp) < FMath::Abs(HitTime - YoungerFrame.Timestamp) ? OlderFrame.bIsInvulnerable : YoungerFrame.bIsInvulnerable;
	InterpolatedFrame.Character = OlderFrame.Character;
	
	for (const auto& YoungerPair : YoungerFrame.HitboxData)
	{
		const FHitboxSnapshot& OlderSnapshot = OlderFrame.HitboxData[YoungerPair.Key];
		const FHitboxSnapshot& YoungerSnapshot = YoungerFrame.HitboxData[YoungerPair.Key];

		FHitboxSnapshot InterpSnapshot;
		InterpSnapshot.Location = FMath::Lerp(OlderSnapshot.Location, YoungerSnapshot.Location, InterpAlpha);
		InterpSnapshot.Rotation = FQuat::Slerp(OlderSnapshot.Rotation, YoungerSnapshot.Rotation, InterpAlpha);
		InterpSnapshot.Extents = YoungerSnapshot.Extents;

		InterpolatedFrame.HitboxData.Add(YoungerPair.Key, InterpSnapshot);
	}

	return InterpolatedFrame;
}

FHitVerificationResult UHitValidationComponent::CheckProjectileCollision(const FCharacterFrameData& FrameData, ADodgerCharacter* TargetCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime) const
{
	FHitVerificationResult Result;

	if (FrameData.Character != TargetCharacter || FrameData.bIsInvulnerable)
	{
		return Result;
	}

	// Save current state
	FCharacterFrameData CurrentFrame;
	CaptureHitboxPositions(TargetCharacter, CurrentFrame);
	
	// Apply rewind frame
	ApplyFrameToCharacter(TargetCharacter, FrameData);

	// Configure hitboxes for collision test
	// Note: if character mesh has collision then need to disable it here and enable at the end
	for (auto& KeyVal : TargetCharacter->GetHitBoxes())
	{
		if (UBoxComponent* Hitbox = KeyVal.Value)
		{
			Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Hitbox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
		}
	}

	// Perform projectile prediction
	const UProjectileConfig* ProjectileSettings = GetDefault<UProjectileConfig>();
	const float Distance = (TraceStart - TargetCharacter->GetActorLocation()).Size();
	const float MaxSimTime = Distance / InitialVelocity.Size() + 1.0f;
	FPredictProjectilePathParams PredictParams(ProjectileSettings->Radius, TraceStart, InitialVelocity, MaxSimTime, ECC_HitBox);
	PredictParams.SimFrequency = 15;
	PredictParams.OverrideGravityZ = GetWorld()->GetWorldSettings()->GetGravityZ() * ProjectileSettings->GravityScale;
	//PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PredictResult;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

	// Check hit results
	if (UBoxComponent* HitHitbox = Cast<UBoxComponent>(PredictResult.HitResult.Component.Get()))
	{
		if (PredictResult.HitResult.bBlockingHit)
		{
			Result.bIsValidHit = TargetCharacter->GetHitBoxes().Contains(HitHitbox->GetFName());
			Result.bIsHeadshot = (HitHitbox->GetFName() == TargetCharacter->GetHitBoxHead()->GetFName());
		}
	}

	// Restore original state
	RestoreCharacterHitboxes(TargetCharacter, CurrentFrame);
	
	return Result;
}

void UHitValidationComponent::CaptureHitboxPositions(ADodgerCharacter* TargetCharacter, FCharacterFrameData& OutFrameData) const
{
	if (!TargetCharacter)
	{
		return;
	}
	
	for (const auto& KeyVal : TargetCharacter->GetHitBoxes())
	{
		if (UBoxComponent* Hitbox = KeyVal.Value)
		{
			FHitboxSnapshot Snapshot;
			Snapshot.Location = Hitbox->GetComponentLocation();
			Snapshot.Rotation = Hitbox->GetComponentQuat();
			Snapshot.Extents = Hitbox->GetScaledBoxExtent();
			OutFrameData.HitboxData.Add(Hitbox->GetFName(), Snapshot);
		}
	}
}

void UHitValidationComponent::ApplyFrameToCharacter(ADodgerCharacter* TargetCharacter, const FCharacterFrameData& FrameData) const
{
	if (TargetCharacter == nullptr)
	{
		return;
	}

	for (auto& KeyVal : TargetCharacter->GetHitBoxes())
	{
		UBoxComponent* Hitbox = KeyVal.Value;
		if (Hitbox && FrameData.HitboxData.Contains(KeyVal.Key))
		{
			const FHitboxSnapshot& Snapshot = FrameData.HitboxData[KeyVal.Key];
			Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Hitbox->SetWorldLocation(Snapshot.Location);
			Hitbox->SetWorldRotation(Snapshot.Rotation);
			Hitbox->SetBoxExtent(Snapshot.Extents);
		}
	}
}

void UHitValidationComponent::RestoreCharacterHitboxes(ADodgerCharacter* TargetCharacter, const FCharacterFrameData& FrameData) const
{
	if (!TargetCharacter)
	{
		return;
	}

	for (auto& KeyVal : TargetCharacter->GetHitBoxes())
	{
		UBoxComponent* Hitbox = KeyVal.Value; 
		if (Hitbox && FrameData.HitboxData.Contains(KeyVal.Key))
		{
			const FHitboxSnapshot& Snapshot = FrameData.HitboxData[KeyVal.Key];
			Hitbox->SetWorldLocation(Snapshot.Location);
			Hitbox->SetWorldRotation(Snapshot.Rotation);
			Hitbox->SetBoxExtent(Snapshot.Extents);
			Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

