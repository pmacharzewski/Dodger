
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DodgerCombatInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UDodgerCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class IDodgerCombatInterface
{
	GENERATED_BODY()

public:
	/**
	 * Set whether the character intends to fire/attack
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetFireIntent(bool bActive);
	/**
	 * Set whether the character intends to dodge
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetDodgeIntent(bool bActive);
	/**
	 * Check if character is currently invulnerable (e.g., during dodge frames)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool IsInvulnerable() const;
	/**
	 * Check if character is currently in a dodge
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool IsDodging() const;
	/**
	 * Check if character is currently attacking
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool IsAttacking() const;
	/**
	 * Get the current combat state
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual float GetHealth() const;
};

inline void IDodgerCombatInterface::SetFireIntent(bool bActive)
{}

inline void IDodgerCombatInterface::SetDodgeIntent(bool bActive)
{}

inline bool IDodgerCombatInterface::IsInvulnerable() const
{
	return false;
}

inline bool IDodgerCombatInterface::IsDodging() const
{
	return false;
}

inline bool IDodgerCombatInterface::IsAttacking() const
{
	return false;
}

inline float IDodgerCombatInterface::GetHealth() const
{
	return 0.0f;
}
