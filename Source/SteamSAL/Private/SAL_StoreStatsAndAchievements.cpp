// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_StoreStatsAndAchievements.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"


USAL_StoreStatsAndAchievements* USAL_StoreStatsAndAchievements::StoreUserStatsAndAchievements(
	const UObject* WorldContextObject)
{
	USAL_StoreStatsAndAchievements* Node = NewObject<USAL_StoreStatsAndAchievements>();
	Node->WorldContextObject = const_cast<UObject*>(WorldContextObject);
	Node->RegisterWithGameInstance(WorldContextObject);

	Node->UserStatsStoredCb.Register(Node, &USAL_StoreStatsAndAchievements::OnUserStatsStored);
	Node->UserAchievementStoredCb.Register(Node, &USAL_StoreStatsAndAchievements::OnUserAchievementStored);
	return Node;
}

void USAL_StoreStatsAndAchievements::Activate()
{
	if (!SteamUserStats() || !SteamUtils())
	{
		OnFailure.Broadcast(TEXT("Steam interfaces unavailable"));
		UnregisterAll();
		return;
	}
	if (!SteamUserStats()->StoreStats())
	{
		OnFailure.Broadcast(TEXT("StoreStats() returned false"));
		UnregisterAll();
		return;
	}
}

void USAL_StoreStatsAndAchievements::OnUserStatsStored(UserStatsStored_t* Cb)
{
	const bool ok = Cb && (Cb->m_nGameID == SteamUtils()->GetAppID()) && (Cb->m_eResult == k_EResultOK);
	AsyncTask(ENamedThreads::GameThread, [this, ok, Cb]()
	{
		if (!ok)
		{
			const FString Err = Cb ? FString::Printf(TEXT("UserStatsStored failed: %d"), int32(Cb->m_eResult))
								   : TEXT("UserStatsStored: no callback");
			OnFailure.Broadcast(Err);
			UnregisterAll();
			return;
		}
		if (!bCompleted) { bCompleted = true; OnSuccess.Broadcast(); UnregisterAll(); }
	});
}

void USAL_StoreStatsAndAchievements::OnUserAchievementStored(UserAchievementStored_t* Cb)
{
	const bool ok = Cb && (Cb->m_nGameID == SteamUtils()->GetAppID());
	AsyncTask(ENamedThreads::GameThread, [this, ok]()
	{
		if (!ok)
		{
			OnFailure.Broadcast(TEXT("UserAchievementStored: wrong AppID or null callback"));
			UnregisterAll();
			return;
		}
		if (!bCompleted) { bCompleted = true; OnSuccess.Broadcast(); UnregisterAll(); }
	});
}
