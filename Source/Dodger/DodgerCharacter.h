// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/DodgerCombatInterface.h"
#include "Logging/LogMacros.h"
#include "DodgerCharacter.generated.h"

class UHitValidationComponent;
class UBoxComponent;
class UDodgerCombatComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ADodgerCharacter : public ACharacter, public IDodgerCombatInterface
{
	GENERATED_BODY()

public:
	ADodgerCharacter();
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UDodgerCombatComponent* GetCombat() const { return CombatComponent; }
	FORCEINLINE UHitValidationComponent* GetHitValidation() const { return HitValidationComponent; }
	FORCEINLINE UBoxComponent* GetHitBoxHead() const { return HitBoxHead; }
	
	FORCEINLINE const TMap<FName, TObjectPtr<UBoxComponent>>& GetHitBoxes() const { return HitBoxes; }
	
	// IDodgerCombatInterface Start
	virtual void SetFireIntent(bool bActive) override;
	virtual void SetDodgeIntent(bool bActive) override;
	virtual bool IsDodging() const override;
	virtual bool IsAttacking() const override;
	virtual bool IsInvulnerable() const override;
	virtual float GetHealth() const override;
	// IDodgerCombatInterface End
	
protected:
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void DodgeStart();
	void DodgeEnd();
	void FireStart();
	void FireStop();
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanJumpInternal_Implementation() const override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:

	void AddHitBox(UBoxComponent* HitBox);
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Doge Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDodgerCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHitValidationComponent> HitValidationComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> HitBoxHead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> HitBoxBody;

	UPROPERTY()
	TMap<FName, TObjectPtr<UBoxComponent>> HitBoxes;
	
	UPROPERTY(Replicated)
	float Health = 100.0f;

	double LastNoiseEmittedTime = 0.0;
};

