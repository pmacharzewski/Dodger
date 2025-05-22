
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DodgerPlayerController.generated.h"

UCLASS()
class DODGER_API ADodgerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * Gets the synchronized server time adjusted for network latency
	 */
	UFUNCTION(BlueprintCallable)
	float GetServerTime() const;
    
protected:
	// Base Class Interface Start
	virtual void ReceivedPlayer() override;
	virtual void Tick(float DeltaSeconds) override;
	// Base Class Interface End
private:
	/**
	 * Server RPC to synchronize time between client and server.
	 * @param ClientTime The client's current time when sending the request
	 */
	UFUNCTION(Server, Reliable)
	void ServerSyncTime(float ClientTime);
	/**
	 * Client RPC to report server time back to the client.
	 * @param ClientTime The original client time from the request
	 * @param ServerTime The server's time when processing the request
	 */
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float ClientTime, float ServerTime);
	/**
	 * Updates the time synchronization logic
	 */
	void UpdateServerTimeSync(float DeltaSeconds);
	/** 
	 * How often to synchronize time with server (in seconds).
	 * Configurable in editor.
	 */
	UPROPERTY(EditAnywhere)
	float TimeSyncFrequency = 3.0f;
	/**
	 * Synchronization cooldown.
	 */
	float TimeSyncRunningTime = 0.0f;
	/**
	 * The calculated time difference between client and server (server - client).
	 * Used to estimate server time locally.
	 */
	float ClientServerDelta = 0.0f;
};
