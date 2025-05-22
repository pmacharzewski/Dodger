
#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "UObject/WeakInterfacePtr.h"
#include "Interfaces/DodgerCombatInterface.h"
#include "EnemyAIController.generated.h"

class UEnemyConfig;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;

enum class EEnemyState : uint8
{
	None,
	Idle,
	Patrol,
	Chase,
	Combat,
};

UCLASS()
class DODGER_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	
	AEnemyAIController();
	
	void SetFollowTarget(AActor* Target);
	
	AActor* GetTargetActor() const;

protected:
	// Base Class Interface Start
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	// Base Class Interface End

	virtual void OnStateEnter(EEnemyState NewState, EEnemyState OldState);
	virtual void OnStateExit(EEnemyState State, EEnemyState NextState);
	
private:

	// State functions
	void SetState(EEnemyState NewState);
	void UpdateIdle(float DeltaTime);
	void UpdateStates(float DeltaTime);
	void UpdatePatrol(float DeltaTime);
	void UpdateChase(float DeltaTime);
	void UpdateCombat(float DeltaTime);

	// Perception callbacks
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	// Helpers
	bool FindPatrolPoint(FVector& OutPatrolPoint) const;
	bool IsTargetValidEnemy(AActor* Actor) const;
	bool HasValidEnemy() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, meta=(AllowPrivateAccess))
	TObjectPtr<const UEnemyConfig> Config;
	
	TWeakObjectPtr<AActor> TargetActor;
	
	FVector BaseLocation = FVector::ZeroVector;
	FVector CurrentPatrolLocation = FVector::ZeroVector;
	
	EEnemyState CurrentState = EEnemyState::None;

	float StateTimeElapsed = 0.0f;
	float TimeToExitIdle = 0.0f;
	float ForcedChaseTimer = 0.0f;
	
	TWeakInterfacePtr<IDodgerCombatInterface> CombatCharacter = nullptr;
};
