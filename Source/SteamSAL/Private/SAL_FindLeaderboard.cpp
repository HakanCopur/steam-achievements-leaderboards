// Fill out your copyright notice in the Description page of Project Settings.

#include "SAL_FindLeaderboard.h"
#include "Async/Async.h"                // for AsyncTask(GameThread,�)
#include "Kismet/GameplayStatics.h"     // for GetGameInstance from WorldContext
#include "Engine/Engine.h"              // for GEngine logging / world access

// Native Steamworks API � defines SteamAPICall_t, SteamUserStats(), CCallResult, etc.
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

USAL_FindLeaderboard* USAL_FindLeaderboard::FindLeaderboard(
	const UObject* WorldContextObject,
	const FString& LeaderboardName)
{
	// 1) create the proxy object (the async “node” instance)
	USAL_FindLeaderboard* Node = NewObject<USAL_FindLeaderboard>();

	// 2) register with the running GameInstance so the node has a stable outer
	//    (prevents premature GC; cleans up automatically on world teardown)
	Node->RegisterWithGameInstance(WorldContextObject);

	// 3) cache inputs for Activate() to use later
	Node->WorldContextObject = const_cast<UObject*>(WorldContextObject); // storing, not mutating
	Node->InLeaderboardName = LeaderboardName;
	// 4) return the proxy; Blueprint holds it and will call Activate() next
	return Node;
}

void USAL_FindLeaderboard::Activate()
{
	//Validating Inputs Early
	if (InLeaderboardName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: empty LeaderboardName"));
		OnFailure.Broadcast(TEXT("LeaderboardName is empty."));
		SetReadyToDestroy(); // tell async framework we’re done
		return;
	}

	//Validate Steam availability before doing anything async
	//SteamUserStats() is nullptr when Steam API isn't initialized/available.
	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: SteamUserStats not available"));
		OnFailure.Broadcast(TEXT("Steam not available or not initialized."));
		SetReadyToDestroy();
		return;
	}
	

	// 4) Kick off the Steam async request (non-blocking)
	const FTCHARToUTF8 NameUTF8(*InLeaderboardName);
	SteamAPICall_t Call = SteamUserStats()->FindLeaderboard(NameUTF8.Get());
	if (Call == k_uAPICallInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] FindLeaderboard: Steam returned invalid APICall"));
		OnFailure.Broadcast(TEXT("Steam returned an invalid API call handle."));
		SetReadyToDestroy();
		return;
	}

	// 5) Bind our completion function to this pending call
	FindLeaderboardCallResult.Set(Call, this, &USAL_FindLeaderboard::OnFindLeaderboardCompleted);

	// 6) Success path will continue in OnFindLeaderboardCompleted(...)
	UE_LOG(LogTemp, Verbose, TEXT("[SAL] FindLeaderboard: request sent for '%s'"), *InLeaderboardName);
}

void USAL_FindLeaderboard::OnFindLeaderboardCompleted(LeaderboardFindResult_t* Result, bool bIOFailure)
{
	// A) transport/network failure or null result
	if (bIOFailure || Result == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] FindLeaderboard: IO failure or null result"));
		OnFailure.Broadcast(TEXT("Steam IO failure during FindLeaderboard."));
		SetReadyToDestroy();
		return;
	}

	// B) logical failure: not found
	if (!Result->m_bLeaderboardFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] FindLeaderboard: leaderboard '%s' not found"), *InLeaderboardName);
		OnFailure.Broadcast(FString::Printf(TEXT("Leaderboard '%s' not found."), *InLeaderboardName));
		SetReadyToDestroy();
		return;
	}

	// C) success: wrap handle for Blueprint clarity
	FSAL_LeaderboardHandle HandleWrapper;
	HandleWrapper.Value = static_cast<int64>(Result->m_hSteamLeaderboard);

	UE_LOG(LogTemp, Log, TEXT("[SAL] FindLeaderboard: found '%s' (Handle=%lld)"), *InLeaderboardName, HandleWrapper.Value);
	OnSuccess.Broadcast(HandleWrapper);

	SetReadyToDestroy();
}



