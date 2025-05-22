
#include "EnemyAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "Data/EnemyConfig.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	Config = GetDefault<UEnemyConfig>();
	
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);

	// apply sensing config
	if (Config->AISenseConfig)
	{
		PerceptionComponent->ConfigureSense(*Config->AISenseConfig);
		PerceptionComponent->SetDominantSense(Config->AISenseConfig->GetSenseImplementation());
	}
	else
	{
		// Configure hearing
		UAISenseConfig_Hearing* FallbackHearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Config_Hearing"));
		FallbackHearingConfig->HearingRange = Config->ChaseStartDistance + 20.0f;
		FallbackHearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		FallbackHearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		FallbackHearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
		PerceptionComponent->ConfigureSense(*FallbackHearingConfig);
		PerceptionComponent->SetDominantSense(FallbackHearingConfig->GetSenseImplementation());
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// cache combat interface
	CombatCharacter = TWeakInterfacePtr<IDodgerCombatInterface>(InPawn);
	
	// set initial position for patrolling around
	BaseLocation = InPawn->GetActorLocation();
	
	// apply custom color
	if (GetCharacter() && GetCharacter()->GetMesh())
	{
		for (int32 MatIdx = 0; MatIdx < GetCharacter()->GetMesh()->GetNumMaterials(); ++MatIdx)
		{
			GetCharacter()->GetMesh()->CreateDynamicMaterialInstance(MatIdx)->SetVectorParameterValue(Config->ColorParamName, Config->EnemyColor);
		}
	}
	
	SetState(EEnemyState::Idle);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CombatCharacter.IsValid() && CombatCharacter->GetHealth() > 0.0f)
	{
		UpdateStates(DeltaTime);

		{ // debug
			static const TMap<EEnemyState, FString> StateToStringMap = {
				{EEnemyState::Idle, "Idle"},
				{EEnemyState::Patrol, "Patrol"},
				{EEnemyState::Chase, "Chase"},
				{EEnemyState::Combat, "Attack"},
			};
		
			DrawDebugString(GetWorld(), FVector(0,-20,150),
				FString::Printf(TEXT("AI State: %s"),
				*StateToStringMap[CurrentState]),
				GetPawn(), FColor::Green, 0, true);
		}
	}
}

void AEnemyAIController::OnStateEnter(EEnemyState NewState, EEnemyState OldState)
{
	StateTimeElapsed = 0.0f;
	
	switch (NewState)
	{
	case EEnemyState::Idle:
		TimeToExitIdle = FMath::FRandRange(Config->RandomIdleTime.X, Config->RandomIdleTime.Y);
		break;
	case EEnemyState::Patrol:
		GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = Config->PatrolWalkSpeed;
		FindPatrolPoint(CurrentPatrolLocation);
		MoveToLocation(CurrentPatrolLocation);
		break;
	case EEnemyState::Chase:
		GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = Config->ChaseWalkSpeed;
		MoveToActor(TargetActor.Get());
		break;
	case EEnemyState::Combat:
		GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 10.0f; //small speed to allow auto rotation
		MoveToActor(TargetActor.Get());
		CombatCharacter->SetFireIntent(true);
		break;
	}
}

void AEnemyAIController::OnStateExit(EEnemyState State, EEnemyState NextState)
{
	switch (State)
	{
	case EEnemyState::Idle:
		StopMovement();
		break;
	case EEnemyState::Patrol:
		StopMovement();
		break;
	case EEnemyState::Chase:
		StopMovement();
		ForcedChaseTimer = 0.0f;
		break;
	case EEnemyState::Combat:
		CombatCharacter->SetFireIntent(false);
		break;
	}
}

void AEnemyAIController::UpdateStates(float DeltaTime)
{
	StateTimeElapsed += DeltaTime;
	
	switch (CurrentState)
	{
	case EEnemyState::Idle:
		UpdateIdle(DeltaTime);
		break;
	case EEnemyState::Patrol:
		UpdatePatrol(DeltaTime);
		break;
	case EEnemyState::Chase:
		UpdateChase(DeltaTime);
		break;
	case EEnemyState::Combat:
		UpdateCombat(DeltaTime);
		break;
	}
}

void AEnemyAIController::SetState(EEnemyState NewState)
{
	if (CurrentState != NewState)
	{
		OnStateExit(CurrentState, NewState);
		EEnemyState OldState = CurrentState;
		CurrentState = NewState;
		OnStateEnter(CurrentState, OldState);
	}
}

void AEnemyAIController::UpdateIdle(float DeltaTime)
{
	if (HasValidEnemy())
	{
		SetState(EEnemyState::Chase);
		return;
	}
	
	if (TimeToExitIdle > StateTimeElapsed)
	{
		SetState(EEnemyState::Patrol);
	}
}

void AEnemyAIController::UpdatePatrol(float DeltaTime)
{
	if (HasValidEnemy())
	{
		SetState(EEnemyState::Chase);
		return;
	}

	// stopped movement - go to idle to decide next step
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		SetState(EEnemyState::Idle);
	}
}

void AEnemyAIController::UpdateChase(float DeltaTime)
{
	// Enemy lost - go to idle
	if (!HasValidEnemy())
	{
		SetState(EEnemyState::Idle);
		return;
	}
	
	const float DistToTarget = GetPawn()->GetSquaredDistanceTo(TargetActor.Get());

	// Reached Attack range - go to attack state
	if (DistToTarget < FMath::Square(Config->AttackRange))
	{
		SetState(EEnemyState::Combat);
		return;
	}

	if ((ForcedChaseTimer -= DeltaTime) <= 0.0f)
	{
		// Player got too far - go back to idle/patrol
		if (DistToTarget > FMath::Square(Config->ChaseStopDistance))
		{
			SetFollowTarget(nullptr);
			SetState(EEnemyState::Idle);
		}
	}
}

void AEnemyAIController::UpdateCombat(float DeltaTime)
{
	if (!HasValidEnemy())
	{
		SetState(EEnemyState::Idle);
		return;
	}

	const float DistToTarget = GetPawn()->GetSquaredDistanceTo(TargetActor.Get());
	
	// Enemy got too far, approach - chase again
	if (DistToTarget > FMath::Square(Config->AttackRange + Config->AttackRangeOffset))
	{
		SetState(EEnemyState::Chase);
		return;
	}

	// Player got too far - go back to idle/patrol
	if (DistToTarget > FMath::Square(Config->ChaseStopDistance))
	{
		SetFollowTarget(nullptr);
		SetState(EEnemyState::Idle);
	}
}

void AEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}
	
	if (!GetCharacter() || Actor == GetCharacter())
	{
		return;
	}
	
	if (Actor == TargetActor)
	{
		return;
	}
	
	if (!IsTargetValidEnemy(Actor))
	{
		return;
	}
	
	// sensed character at chase distance - got to Chase
	if (GetCharacter()->GetDistanceTo(Actor) < Config->ChaseStartDistance)
	{
		SetFollowTarget(Actor);
	}
	else if (CurrentState == EEnemyState::Patrol)
	{
		// force chase - could be a projectile caused the noise
		if (FVector::Distance(Stimulus.StimulusLocation, GetCharacter()->GetActorLocation()) < Config->ChaseStartDistance)
		{
			ForcedChaseTimer = 5.0f;
			SetFollowTarget(Actor);
		}
	}
}

bool AEnemyAIController::FindPatrolPoint(FVector& OutPatrolPoint) const
{
	// Set first patrol point
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation Result;
		if (NavSys->GetRandomReachablePointInRadius(BaseLocation, Config->PatrolRange, Result))
		{
			OutPatrolPoint = Result.Location;
			return true;
		}
	}

	return false;
}

bool AEnemyAIController::IsTargetValidEnemy(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (IDodgerCombatInterface* DodgerEnemy = Cast<IDodgerCombatInterface>(Actor))
	{
		return DodgerEnemy->GetHealth() > 0.0f;
	}

	return false;
}

bool AEnemyAIController::HasValidEnemy() const
{
	return IsTargetValidEnemy(TargetActor.Get());
}


void AEnemyAIController::SetFollowTarget(AActor* Target)
{
	TargetActor = Target;
}

AActor* AEnemyAIController::GetTargetActor() const
{
	return TargetActor.Get();	
}
