// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_RequestCurrentStats.h"
#include "OnlineSubsystem.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "SALTypes.h"

USAL_RequestCurrentStats* USAL_RequestCurrentStats::RequestCurrentStats(UObject* WorldContextObject)
{
	USAL_RequestCurrentStats* Node = NewObject<USAL_RequestCurrentStats>();
	Node->WorldContextObject = WorldContextObject;
	Node->RegisterWithGameInstance(WorldContextObject);

	UE_LOG(LogSteamSAL, Log,
	       TEXT("[SAL_RequestCurrentStatsAchievements] Node created and registered with GameInstance."));

	return Node;
}

void USAL_RequestCurrentStats::Activate()
{
	const bool bHasUser = (SteamUser() != nullptr);
	const bool bHasUserStats = (SteamUserStats() != nullptr);
	const bool bHasUtils = (SteamUtils() != nullptr);

	const uint32 AppID = SteamUtils() ? SteamUtils()->GetAppID() : 0;
	const bool bLoggedOn = SteamUser() ? SteamUser()->BLoggedOn() : false;
	const bool bAvailable = bHasUser && bHasUserStats && bHasUtils && (AppID != 0) && bLoggedOn;

	const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	const FString OSSName = OSS ? OSS->GetSubsystemName().ToString() : TEXT("<null>");
	UE_LOG(LogTemp, Log, TEXT("[SAL_RequestCurrentStatsAchievements] IsSteamAvailable() = %s"),
	       bAvailable ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log,
	       TEXT("[SAL_RequestCurrentStatsAchievements] OSS=%s  AppID=%u  SteamUserStats=%s  SteamUser=%s  LoggedOn=%s"),
	       *OSSName,
	       AppID,
	       bHasUserStats ? TEXT("OK") : TEXT("null"),
	       bHasUser ? TEXT("OK") : TEXT("null"),
	       bLoggedOn ? TEXT("true") : TEXT("false"));

	if (!SteamUserStats() || !SteamUser())
	{
		UE_LOG(LogSteamSAL, Log, TEXT("[SAL_RequestCurrentStatsAchievements] Steam interfaces unavailable."));
		OnFailure.Broadcast(TEXT("Steam interfaces unavailable"));
		return;
	}


	const SteamAPICall_t Call = SteamUserStats()->RequestUserStats(SteamUser()->GetSteamID());
	UE_LOG(LogSteamSAL, Log,
	       TEXT("[SAL_RequestCurrentStatsAchievements] RequestCurrentStatsAchievements called, SteamAPICall_t=%llu"),
	       Call);

	CallResult.Set(Call, this, &USAL_RequestCurrentStats::OnUserStatsReceived);
}

void USAL_RequestCurrentStats::OnUserStatsReceived(UserStatsReceived_t* Callback, bool bIOFailure)
{
	UE_LOG(LogSteamSAL, Log, TEXT("[SAL_RequestCurrentStatsAchievements] OnUserStatsReceived fired. bIOFailure=%s"),
	       bIOFailure ? TEXT("true") : TEXT("false"));

	if (Callback)
	{
		UE_LOG(LogSteamSAL, Log, TEXT("[SAL_RequestCurrentStatsAchievements] Callback GameID=%llu, Result=%d"),
		       Callback->m_nGameID, (int32)Callback->m_eResult);
	}
	else
	{
		UE_LOG(LogSteamSAL, Error, TEXT("[SAL_RequestCurrentStatsAchievements] Callback pointer was null!"));
	}

	const bool bOK = (!bIOFailure) &&
		Callback &&
		(Callback->m_nGameID == SteamUtils()->GetAppID()) &&
		(Callback->m_eResult == k_EResultOK);

	AsyncTask(ENamedThreads::GameThread, [this, bOK, Callback]()
	{
		if (bOK)
		{
			UE_LOG(LogSteamSAL, Log, TEXT("[SAL_RequestCurrentStatsAchievements] Success — stats are ready."));
			OnSuccess.Broadcast();
		}
		else
		{
			const FString Err = Callback
				                    ? FString::Printf(
					                    TEXT("RequestCurrentStatsAchievements failed: %d"), int32(Callback->m_eResult))
				                    : TEXT("Achievements failed: no callback");

			UE_LOG(LogSteamSAL, Warning, TEXT("[SAL_RequestCurrentStatsAchievements] Failure — %s"), *Err);
			OnFailure.Broadcast(Err);
		}
	});
}
