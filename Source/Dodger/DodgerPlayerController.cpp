
#include "DodgerPlayerController.h"

float ADodgerPlayerController::GetServerTime() const
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ADodgerPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// Request sync time immediately when player is valid
	if (IsLocalController())
	{
		ServerSyncTime(GetWorld()->GetTimeSeconds());
	}
}

void ADodgerPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateServerTimeSync(DeltaSeconds);
}

void ADodgerPlayerController::ServerSyncTime_Implementation(float ClientTime)
{
	// Get current server time and report back to client
	const float ServerTime = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(ClientTime, ServerTime);
}

void ADodgerPlayerController::ClientReportServerTime_Implementation(float ClientTime, float ServerTime)
{
	// Calculate round trip network delay
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - ClientTime;
	
	// Estimate one-way trip time (half of round trip)
	const float SingleTripTime = 0.5f * RoundTripTime;

	// Estimate current server time accounting for network latency
	const float CurrentServerTime = ServerTime + SingleTripTime;

	// Update time difference between client and server
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ADodgerPlayerController::UpdateServerTimeSync(float DeltaSeconds)
{
	if (IsLocalController())
	{
		if ((TimeSyncRunningTime += DeltaSeconds) > TimeSyncFrequency)
		{
			ServerSyncTime(GetWorld()->GetTimeSeconds());
			TimeSyncRunningTime = FMath::FRandRange(-1.0f, 1.0f);
		}
	}
}
