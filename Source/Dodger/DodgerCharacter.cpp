// Copyright Epic Games, Inc. All Rights Reserved.

#include "DodgerCharacter.h"

#include "Dodger/HitValidationTypes.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/BoxComponent.h"
#include "Components/DodgerCombatComponent.h"
#include "Components/HitValidationComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ADogerCharacter
ADodgerCharacter::ADodgerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // ...and rotates to face that direction
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	CombatComponent = CreateDefaultSubobject<UDodgerCombatComponent>(TEXT("CombatComponent"));
	HitValidationComponent = CreateDefaultSubobject<UHitValidationComponent>(TEXT("ReconcilerComponent"));
	
	HitBoxHead = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBoxHead"));
	HitBoxHead->SetupAttachment(GetMesh(), FName("head"));
	HitBoxHead->InitBoxExtent(FVector(16.0));
	AddHitBox(HitBoxHead);
	
	HitBoxBody = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBoxBody"));
	HitBoxBody->SetupAttachment(GetMesh(), FName("pelvis"));
	HitBoxBody->InitBoxExtent(FVector(75, 25, 32));
	HitBoxBody->SetRelativeLocation(FVector(-20.0, 0.0, 0.0));
	AddHitBox(HitBoxBody);
	
}

void ADodgerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enable hit boxes on server for verification projectile hits.
	if (HasAuthority())
	{
		for (auto& HitBoxPair : HitBoxes)
		{
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}

void ADodgerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	CombatComponent->UpdateCombat(DeltaSeconds);

	// emmit noise so enemy can hear us
	if (HasAuthority())
	{
		if (GetWorld()->TimeSince(LastNoiseEmittedTime) > 1.0)
		{
			if (GetVelocity().SizeSquared2D() > 1.0f)
			{
				LastNoiseEmittedTime = GetWorld()->GetTimeSeconds();
				MakeNoise(1, this, GetActorLocation());
			}
		}
	}
}

bool ADodgerCharacter::CanJumpInternal_Implementation() const
{
	return !IsDodging() && Super::CanJumpInternal_Implementation();
}

float ADodgerCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	// Is final hit
	if (Health > 0.0f && (Health -= DamageTaken) <= 0.0f)
	{
		const FVector Direction = ((GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() + FVector::UpVector) * 0.5f;
		CombatComponent->NetMultiServeFinalBlow(Direction);

		// leave body and allow free fly mode
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->StartSpectatingOnly();	
		}
	}
	
	return DamageTaken;
}

void ADodgerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADodgerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADodgerCharacter::Look);

		// Dodging
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ADodgerCharacter::DodgeStart);
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Completed, this, &ADodgerCharacter::DodgeEnd);
		
		// Fire
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ADodgerCharacter::FireStart);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ADodgerCharacter::FireStop);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ADodgerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ADodgerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ADodgerCharacter::DodgeStart()
{
	SetDodgeIntent(true);
}

void ADodgerCharacter::DodgeEnd()
{
	SetDodgeIntent(false);
}

void ADodgerCharacter::FireStart()
{
	SetFireIntent(true);
}

void ADodgerCharacter::FireStop()
{
	SetFireIntent(false);
}

void ADodgerCharacter::AddHitBox(UBoxComponent* HitBox)
{
	HitBox->SetCollisionObjectType(ECC_HitBox);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBoxes.Add(HitBox->GetFName(), HitBox);
}

bool ADodgerCharacter::IsInvulnerable() const
{
	return CombatComponent->IsInvulnerable();
}

void ADodgerCharacter::SetFireIntent(bool bActive)
{
	CombatComponent->SetFireIntent(bActive);
}

void ADodgerCharacter::SetDodgeIntent(bool bActive)
{
	CombatComponent->SetDodgeIntent(bActive);
}

bool ADodgerCharacter::IsDodging() const
{
	return CombatComponent->IsDodging();
}

bool ADodgerCharacter::IsAttacking() const
{
	return CombatComponent->IsAttacking();
}

float ADodgerCharacter::GetHealth() const
{
	return Health;
}

void ADodgerCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADodgerCharacter, Health);
}