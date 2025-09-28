// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_RequestGlobalStats.h"

USAL_RequestGlobalStats* USAL_RequestGlobalStats::RequestGlobalStats(int32 Days)
{
	USAL_RequestGlobalStats* Node = NewObject<USAL_RequestGlobalStats>();
	Node->RequestedDays = FMath::Max(0, Days);
	return Node;
}

void USAL_RequestGlobalStats::Activate()
{
	if (SteamUserStats() == nullptr)
	{
		OnFailure.Broadcast();
		SetReadyToDestroy();
		return;
	}

	SteamAPICall_t Handle = SteamUserStats()->RequestGlobalStats(RequestedDays);
	if (Handle == k_uAPICallInvalid)
	{
		OnFailure.Broadcast();
		SetReadyToDestroy();
		return;
	}

	CallResult.Set(Handle, this, &USAL_RequestGlobalStats::OnGlobalStatsReceived);
}

void USAL_RequestGlobalStats::OnGlobalStatsReceived(GlobalStatsReceived_t* Callback, bool bIOFailure)
{
	bool bOk = !bIOFailure
		&& Callback
		&& Callback->m_eResult == k_EResultOK;

	if (bOk && SteamUtils())
	{
		AppId_t MyApp = SteamUtils()->GetAppID();
		if (Callback->m_nGameID != (uint64)MyApp)
		{
			bOk = false;
		}
	}

	if (bOk)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}

