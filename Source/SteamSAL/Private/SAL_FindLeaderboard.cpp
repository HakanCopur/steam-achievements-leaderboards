// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_FindLeaderboard.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

USAL_FindLeaderboard* USAL_FindLeaderboard::FindLeaderboard(
	UObject* WorldContextObject,
	const FString& LeaderboardName)
{
	USAL_FindLeaderboard* Node = NewObject<USAL_FindLeaderboard>();

	Node->RegisterWithGameInstance(WorldContextObject);

	Node->WorldContextObject = WorldContextObject;
	Node->InLeaderboardName = LeaderboardName;

	return Node;
}

void USAL_FindLeaderboard::Activate()
{
	if (InLeaderboardName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: empty LeaderboardName"));
		OnFailure.Broadcast(TEXT("LeaderboardName is empty."));
		SetReadyToDestroy();
		return;
	}


	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: SteamUserStats not available"));
		OnFailure.Broadcast(TEXT("Steam not available or not initialized."));
		SetReadyToDestroy();
		return;
	}
	

	const FTCHARToUTF8 NameUTF8(*InLeaderboardName);
	SteamAPICall_t Call = SteamUserStats()->FindLeaderboard(NameUTF8.Get());
	if (Call == k_uAPICallInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] FindLeaderboard: Steam returned invalid APICall"));
		OnFailure.Broadcast(TEXT("Steam returned an invalid API call handle."));
		SetReadyToDestroy();
		return;
	}

	FindLeaderboardCallResult.Set(Call, this, &USAL_FindLeaderboard::OnFindLeaderboardCompleted);

	UE_LOG(LogTemp, Verbose, TEXT("[SAL] FindLeaderboard: request sent for '%s'"), *InLeaderboardName);
}

void USAL_FindLeaderboard::OnFindLeaderboardCompleted(LeaderboardFindResult_t* Result, bool bIOFailure)
{
	if (bIOFailure || Result == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] FindLeaderboard: IO failure or null result"));
		OnFailure.Broadcast(TEXT("Steam IO failure during FindLeaderboard."));
		SetReadyToDestroy();
		return;
	}

	if (!Result->m_bLeaderboardFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: leaderboard '%s' not found"), *InLeaderboardName);
		OnFailure.Broadcast(FString::Printf(TEXT("Leaderboard '%s' not found."), *InLeaderboardName));
		SetReadyToDestroy();
		return;
	}

	FSAL_LeaderboardHandle HandleWrapper;
	HandleWrapper.Value = static_cast<int64>(Result->m_hSteamLeaderboard);

	UE_LOG(LogTemp, Log, TEXT("[SAL] FindLeaderboard: found '%s' (Handle=%lld)"), *InLeaderboardName, HandleWrapper.Value);
	OnSuccess.Broadcast(HandleWrapper);

	SetReadyToDestroy();
}



