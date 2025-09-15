// Fill out your copyright notice in the Description page of Project Settings.


#include "SAL_UploadScore.h"

USAL_UploadScore* USAL_UploadScore::UploadScore(const UObject* WorldContextObject,
                                                FSAL_LeaderboardHandle LeaderboardHandle,
                                                int32 InScore, ESALLeaderboardUploadMethod InMethod,
                                                const TArray<int32>& Details)
{
	USAL_UploadScore* Node = NewObject<USAL_UploadScore>();

	Node->RegisterWithGameInstance(WorldContextObject);

	Node->WorldContextObject = const_cast<UObject*>(WorldContextObject);
	Node->LeaderboardHandle = LeaderboardHandle;
	Node->Score = InScore;
	Node->UploadMethod = InMethod;
	Node->InDetails  = Details;  


	return Node;
}

void USAL_UploadScore::Activate()
{
	if (LeaderboardHandle.Value == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] UploadScore: Invalid LeaderboardHandle"));
		OnFailure.Broadcast(TEXT("Invalid LeaderboardHandle"));
		SetReadyToDestroy();
		return;
	}

	//Validate Steam availability before doing anything async
	//SteamUserStats() is nullptr when Steam API isn't initialized/available.
	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] UploadScore: SteamUserStats not available"));
		OnFailure.Broadcast(TEXT("Steam not available or not initialized."));
		SetReadyToDestroy();
		return;
	}

	const SteamLeaderboard_t SteamHandle = static_cast<SteamLeaderboard_t>(LeaderboardHandle.Value);
	const ELeaderboardUploadScoreMethod Method =
		(UploadMethod == ESALLeaderboardUploadMethod::ForceUpdate)
			? k_ELeaderboardUploadScoreMethodForceUpdate
			: k_ELeaderboardUploadScoreMethodKeepBest;

	int32 DetailsCount = FMath::Clamp(InDetails.Num(), 0, 64);
	const int32* DetailsPtr = (DetailsCount > 0) ? InDetails.GetData() : nullptr;

	SteamAPICall_t Call = SteamUserStats()->UploadLeaderboardScore(
		SteamHandle,
		Method,
		Score,
		DetailsPtr,
		DetailsCount
	);
	if (Call == k_uAPICallInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] UploadScore: Steam returned invalid APICall"));
		OnFailure.Broadcast(TEXT("Steam returned an invalid API call handle."));
		SetReadyToDestroy();
		return;
	}

	UploadCallResult.Set(Call, this, &USAL_UploadScore::OnUploadCompleted);

	UE_LOG(LogTemp, Verbose, TEXT("[SAL] UploadScore: sent (Handle=%llu, Score=%d, Method=%d)"),
	       (unsigned long long)LeaderboardHandle.Value, Score, (int32)Method);
}

void USAL_UploadScore::OnUploadCompleted(LeaderboardScoreUploaded_t* Result, bool bIOFailure)
{
	// 1) transport-level failure or null result
	if (bIOFailure || Result == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] UploadScore: IO failure or null result"));
		OnFailure.Broadcast(TEXT("Steam IO failure during UploadLeaderboardScore."));
		SetReadyToDestroy();
		return;
	}

	// 2) logical failure returned by Steam
	if (Result->m_bSuccess == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] UploadScore: Steam reported upload failure (Score=%d)"), Result->m_nScore);
		OnFailure.Broadcast(TEXT("UploadLeaderboardScore failed."));
		SetReadyToDestroy();
		return;
	}

	// 3) optional: warn if callback handle doesn't match our cached handle
	const SteamLeaderboard_t Expected = static_cast<SteamLeaderboard_t>(LeaderboardHandle.Value);
	if (Result->m_hSteamLeaderboard != Expected)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] UploadScore: callback handle mismatch (Expected=%llu, Got=%llu)"),
		       static_cast<unsigned long long>(Expected),
		       static_cast<unsigned long long>(Result->m_hSteamLeaderboard));
	}

	// 4) success: extract fields and broadcast
	const bool bScoreChanged = (Result->m_bScoreChanged != 0);
	const int32 NewGlobalRank = Result->m_nGlobalRankNew;
	const int32 PreviousGlobalRank = Result->m_nGlobalRankPrevious;

	UE_LOG(LogTemp, Log, TEXT("[SAL] UploadScore: success (Score=%d, Changed=%s, NewRank=%d, PrevRank=%d)"),
	       Result->m_nScore,
	       bScoreChanged ? TEXT("true") : TEXT("false"),
	       NewGlobalRank, PreviousGlobalRank);

	OnSuccess.Broadcast();

	// 5) we're done; allow GC to clean up the proxy
	SetReadyToDestroy();
}
