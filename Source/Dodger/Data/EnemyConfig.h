#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyConfig.generated.h"

class UAISenseConfig;

UCLASS()
class UEnemyConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    /**
     * Random time range (in seconds) the enemy will wait between idle behaviors.
     * X = Minimum idle time, Y = Maximum idle time.
     */
    UPROPERTY(EditAnywhere, Category = "Idle")
    FVector2D RandomIdleTime = {0.5f, 1.0f};
    /**
     * Maximum distance from spawn point that the enemy can patrol.
     * Enemy will choose random points within this radius when patrolling.
     */
    UPROPERTY(EditAnywhere, Category = "Patrol")
    float PatrolRange = 1000.0f;
    /**
     * Movement speed when enemy is in patrol state.
     */
    UPROPERTY(EditAnywhere, Category = "Patrol")
    float PatrolWalkSpeed = 200.0f;
    /**
     * Distance at which enemy will start chasing the player.
     */
    UPROPERTY(EditAnywhere, Category = "Chase")
    float ChaseStartDistance = 800.0f;
    /**
     * Distance at which enemy will stop chasing the player.
     */
    UPROPERTY(EditAnywhere, Category = "Chase")
    float ChaseStopDistance = 1200.0f;
    /**
     * Movement speed when enemy is chasing the player.
     */
    UPROPERTY(EditAnywhere, Category = "Chase")
    float ChaseWalkSpeed = 400.0f;
    /**
     * Distance at which enemy enters fighting state.
     */
    UPROPERTY(EditAnywhere, Category = "Attack")
    float AttackRange = 400.0f;
    /**
     * Acceptable variance around AttackRange for attacking.
     * Enemy can attack when within AttackRange + AttackRangeOffset once attack started.
     */
    UPROPERTY(EditAnywhere, Category = "Attack")
    float AttackRangeOffset = 100.0f;
    /**
     * Primary color used for this enemy body.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
    FLinearColor EnemyColor = FLinearColor::Red;
    /**
     * Name of the material parameter controlling color/tint.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
    FName ColorParamName = TEXT("Tint");
    /**
     * Configuration for AI perception (sight, hearing, etc.).
     */
    UPROPERTY(EditAnywhere, Instanced, Category = "Visuals")
    TObjectPtr<UAISenseConfig> AISenseConfig;
};