#pragma once
#include "CoreMinimal.h"

#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel1

class ADodgerCharacter;

struct FHitboxSnapshot
{
	FVector Location = FVector::ZeroVector;
	
	FQuat Rotation = FQuat::Identity;
	
	FVector Extents = FVector::ZeroVector;
};

struct FCharacterFrameData
{
	TMap<FName, FHitboxSnapshot> HitboxData = {};
	
	TWeakObjectPtr<ADodgerCharacter> Character = nullptr;

	float Timestamp = 0.0f;
	
	bool bIsInvulnerable = false;
};

struct FHitVerificationResult
{
	bool bIsValidHit : 1;
	
	bool bIsHeadshot : 1;
};