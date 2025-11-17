// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_DownloadLeaderboardEntries.h"
#include "Misc/EngineVersionComparison.h"
#include "SAL_Internal.h"

USAL_DownloadLeaderboardEntries* USAL_DownloadLeaderboardEntries::DownloadLeaderboardEntries(
	UObject* WorldContextObject, FSAL_LeaderboardHandle LeaderboardHandle, ELeaderboardRequestType RequestType,
	int32 RangeStart, int32 RangeEnd)
{
	USAL_DownloadLeaderboardEntries* Node = NewObject<USAL_DownloadLeaderboardEntries>();

	Node->RegisterWithGameInstance(WorldContextObject);

	Node->WorldContextObject = WorldContextObject;
	Node->InHandle = LeaderboardHandle;
	Node->InRequestType = RequestType;
	Node->InRangeStart = RangeStart;
	Node->InRangeEnd = RangeEnd;
	Node->InDetailsMax = 64;

	return Node;
}

void USAL_DownloadLeaderboardEntries::Activate()
{
	if (InHandle.Value == 0)
	{
		Fail(TEXT("Invalid LeaderboardHandle. Make sure FindLeaderboard succeeded."));
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		Fail(TEXT("SteamUserStats not available or not initialized."));
		return;
	}

	if (InRequestType != ELeaderboardRequestType::Friends && InRangeEnd < InRangeStart)
	{
		Fail(FString::Printf(TEXT("Invalid range [%d..%d]."), InRangeStart, InRangeEnd));
		return;
	}

	ELeaderboardDataRequest DataRequest = k_ELeaderboardDataRequestGlobal;
	switch (InRequestType)
	{
	case ELeaderboardRequestType::Global:
		DataRequest = k_ELeaderboardDataRequestGlobal;
		break;

	case ELeaderboardRequestType::GlobalAroundUser:
		DataRequest = k_ELeaderboardDataRequestGlobalAroundUser;
		break;

	case ELeaderboardRequestType::Friends:
		DataRequest = k_ELeaderboardDataRequestFriends;
		break;

	default:
		DataRequest = k_ELeaderboardDataRequestGlobal;
		break;
	}

	SteamLeaderboard_t SteamHandle = static_cast<SteamLeaderboard_t>(InHandle.Value);

	const int32 Start = (InRequestType == ELeaderboardRequestType::Friends) ? 0 : InRangeStart;
	const int32 End = (InRequestType == ELeaderboardRequestType::Friends) ? 0 : InRangeEnd;

	SteamAPICall_t APICall = SteamUserStats()->DownloadLeaderboardEntries(
		SteamHandle,
		DataRequest,
		Start,
		End
	);

	if (APICall == k_uAPICallInvalid)
	{
		Fail(TEXT("Steam returned an invalid API call handle."));
		return;
	}

	CallResult.Set(APICall, this, &USAL_DownloadLeaderboardEntries::OnScoresDownloaded);
}

void USAL_DownloadLeaderboardEntries::OnScoresDownloaded(LeaderboardScoresDownloaded_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		Fail(TEXT("Steam leaderboard download failed (IO failure)."));
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		Fail(TEXT("SteamUserStats not available in callback."));
		return;
	}

	FSAL_LeaderboardEntriesData EntriesData;
	EntriesData.RequestType = InRequestType;
	EntriesData.TotalEntryCount = Callback->m_cEntryCount;

	if (Callback->m_cEntryCount > 0)
	{
		EntriesData.Entries.Reserve(Callback->m_cEntryCount);
	}

	if (Callback->m_cEntryCount <= 0)
	{
		TWeakObjectPtr<USAL_DownloadLeaderboardEntries> Self(this);

		SAL_RunOnGameThread([Self, EntriesData]()
		{
			if (!Self.IsValid()) return;
			const int32 EntryCount = EntriesData.Entries.Num();
			Self->OnSuccess.Broadcast(EntriesData, EntryCount);
			Self->SetReadyToDestroy();
		});

		return;
	}

	for (int32 i = 0; i < Callback->m_cEntryCount; ++i)
	{
		LeaderboardEntry_t Entry;

		TArray<int32> DetailsBuffer;
		int32* DetailsPtr = nullptr;

		if (InDetailsMax > 0)
		{
			DetailsBuffer.SetNumUninitialized(InDetailsMax);
			DetailsPtr = DetailsBuffer.GetData();
		}

		const bool bGot = SteamUserStats()->GetDownloadedLeaderboardEntry(
			Callback->m_hSteamLeaderboardEntries,
			i,
			&Entry,
			DetailsPtr,
			InDetailsMax
		);

		if (!bGot)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SteamSAL] Failed to get entry %d"), i);
			continue;
		}

		FSAL_LeaderboardEntryRow Row;
		Row.SteamID = LexToString(Entry.m_steamIDUser.ConvertToUint64());
		Row.GlobalRank = Entry.m_nGlobalRank;
		Row.Score = Entry.m_nScore;
		Row.UGCHandle.Value = static_cast<int64>(Entry.m_hUGC);
		Row.bHasUGC = (Entry.m_hUGC != 0);

		if (SteamFriends())
		{
			const char* Nick = SteamFriends()->GetFriendPersonaName(Entry.m_steamIDUser);
			Row.PlayerName = Nick ? UTF8_TO_TCHAR(Nick) : TEXT("");

			if (Row.PlayerName.IsEmpty())
			{
				SteamFriends()->RequestUserInformation(Entry.m_steamIDUser, true);
			}
		}

		if (InDetailsMax > 0)
		{
			const int32 ActualDetails = FMath::Min(Entry.m_cDetails, InDetailsMax);

#if UE_VERSION_OLDER_THAN(5, 4, 0)
			DetailsBuffer.SetNum(ActualDetails, false);
#else
			DetailsBuffer.SetNum(ActualDetails, EAllowShrinking::No);
#endif

			Row.Details = MoveTemp(DetailsBuffer);
		}

		EntriesData.Entries.Add(MoveTemp(Row));
	}

	TWeakObjectPtr<USAL_DownloadLeaderboardEntries> Self(this);

	SAL_RunOnGameThread([Self, EntriesData = MoveTemp(EntriesData)]() mutable
	{
		if (!Self.IsValid()) return;

		const int32 EntryCount = EntriesData.Entries.Num();
		Self->OnSuccess.Broadcast(EntriesData, EntryCount);
		Self->SetReadyToDestroy();
	});
}


void USAL_DownloadLeaderboardEntries::Fail(const FString& Why)
{
	const FString WhyCopy = Why;
	TWeakObjectPtr<USAL_DownloadLeaderboardEntries> Self(this);

	SAL_RunOnGameThread([Self, WhyCopy]()
	{
		if (!Self.IsValid()) return;
		Self->OnFailure.Broadcast(WhyCopy);
		Self->SetReadyToDestroy();
	});
}
