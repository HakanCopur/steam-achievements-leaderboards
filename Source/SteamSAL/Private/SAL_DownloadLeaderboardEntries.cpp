// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_DownloadLeaderboardEntries.h"


USAL_DownloadLeaderboardEntries* USAL_DownloadLeaderboardEntries::DownloadLeaderboardEntries(
	const UObject* WorldContextObject, FSAL_LeaderboardHandle LeaderboardHandle, ELeaderboardRequestType RequestType,
	int32 RangeStart, int32 RangeEnd, int32 DetailsMax)
{
	USAL_DownloadLeaderboardEntries* Node = NewObject<USAL_DownloadLeaderboardEntries>();

	Node->RegisterWithGameInstance(WorldContextObject);

	Node->WorldContextObject = const_cast<UObject*>(WorldContextObject);
	Node->InHandle = LeaderboardHandle;
	Node->InRequestType = RequestType;
	Node->InRangeStart = RangeStart;
	Node->InRangeEnd = RangeEnd;
	Node->InDetailsMax = DetailsMax;

	return Node;
}

void USAL_DownloadLeaderboardEntries::Activate()
{
	if (InHandle.Value == 0)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "[SAL] DownloadLeaderboardEntries: Invalid LeaderboardHandle (Value==0). Call FindLeaderboard first."
		       ));
		Fail(TEXT("Invalid LeaderboardHandle. Make sure FindLeaderboard succeeded."));
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadLeaderboardEntries: SteamUserStats not available"));
		Fail(TEXT("Steam not available or not initialized."));
		return;
	}

	if (InRequestType != ELeaderboardRequestType::Friends && InRangeEnd < InRangeStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadLeaderboardEntries: Bad range [%d..%d]"), InRangeStart,
		       InRangeEnd);
		Fail(FString::Printf(TEXT("Bad range [%d..%d]."), InRangeStart, InRangeEnd));
		return;
	}

	if (InDetailsMax < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadLeaderboardEntries: DetailsMax cannot be negative"));
		Fail(TEXT("DetailsMax cannot be negative."));
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[SAL] DownloadLeaderboardEntries: inputs valid; ready to issue Steam call"));

	ELeaderboardDataRequest DataReq = k_ELeaderboardDataRequestGlobal;
	switch (InRequestType)
	{
	case ELeaderboardRequestType::Global: DataReq = k_ELeaderboardDataRequestGlobal;
		break;
	case ELeaderboardRequestType::GlobalAroundUser: DataReq = k_ELeaderboardDataRequestGlobalAroundUser;
		break;
	case ELeaderboardRequestType::Friends: DataReq = k_ELeaderboardDataRequestFriends;
		break;
	default: DataReq = k_ELeaderboardDataRequestGlobal;
		break;
	}

	const SteamLeaderboard_t SteamHandle = static_cast<SteamLeaderboard_t>(InHandle.Value);

	const int32 Start = InRequestType == ELeaderboardRequestType::Friends ? 0 : InRangeStart;
	const int32 End = InRequestType == ELeaderboardRequestType::Friends ? 0 : InRangeEnd;

	SteamAPICall_t Call = SteamUserStats()->DownloadLeaderboardEntries(
		SteamHandle,
		DataReq,
		Start,
		End
	);

	if (Call == k_uAPICallInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] DownloadLeaderboardEntries: invalid APICall"));
		Fail(TEXT("Steam returned an invalid API call handle."));
		return;
	}

	CallResult.Set(Call, this, &USAL_DownloadLeaderboardEntries::OnScoresDownloaded);

	UE_LOG(LogTemp, Verbose, TEXT("[SAL] DownloadLeaderboardEntries: request sent (Type=%d, %d..%d)"),
	       static_cast<int32>(InRequestType), Start, End);
}


void USAL_DownloadLeaderboardEntries::OnScoresDownloaded(LeaderboardScoresDownloaded_t* Callback, bool bIOFailure)
{
    if (bIOFailure || Callback == nullptr)
    {
        Fail(TEXT("[SAL] DownloadLeaderboardEntries: IO failure or null callback"));
        return;
    }

    if (SteamUserStats() == nullptr)
    {
        Fail(TEXT("[SAL] DownloadLeaderboardEntries: SteamUserStats not available in callback"));
        return;
    }

    InDetailsMax = FMath::Clamp(InDetailsMax, 0, 64);

    if (Callback->m_cEntryCount <= 0)
    {
        TArray<FSAL_LeaderboardEntryRow> Empty;
        OnSuccess.Broadcast(Empty);
        SetReadyToDestroy();
        return;
    }

    const SteamLeaderboard_t Expected = static_cast<SteamLeaderboard_t>(InHandle.Value);
    if (Callback->m_hSteamLeaderboard != Expected)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadLeaderboardEntries: callback for a different leaderboard handle"));
    }

    TArray<FSAL_LeaderboardEntryRow> Rows;
    Rows.Reserve(Callback->m_cEntryCount);

    for (int32 i = 0; i < Callback->m_cEntryCount; ++i)
    {
        LeaderboardEntry_t Entry;

        TArray<int32> DetailsTmp;
        int32* DetailsPtr = nullptr;
        if (InDetailsMax > 0)
        {
            DetailsTmp.SetNumUninitialized(InDetailsMax);
            DetailsPtr = DetailsTmp.GetData();
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
            UE_LOG(LogTemp, Warning, TEXT("[SAL] GetDownloadedLeaderboardEntry failed at index %d"), i);
            continue;
        }

        FSAL_LeaderboardEntryRow Row;
        Row.SteamID  = LexToString(Entry.m_steamIDUser.ConvertToUint64());
        Row.GlobalRank = Entry.m_nGlobalRank;
        Row.Score      = Entry.m_nScore;

    	if (SteamFriends())
    	{
    		const char* Persona = SteamFriends()->GetFriendPersonaName(Entry.m_steamIDUser);
    		Row.PlayerName = Persona ? UTF8_TO_TCHAR(Persona) : TEXT("");

    		if (Row.PlayerName.IsEmpty())
    		{
    			SteamFriends()->RequestUserInformation(Entry.m_steamIDUser, true);
    		}
    	}

        if (InDetailsMax > 0)
        {
            const int32 Actual = FMath::Min(Entry.m_cDetails, InDetailsMax);
			DetailsTmp.SetNum(Actual, EAllowShrinking::No);
            Row.Details = MoveTemp(DetailsTmp);
        }

        Rows.Add(MoveTemp(Row));
    }

    OnSuccess.Broadcast(Rows);
    SetReadyToDestroy();
}


void USAL_DownloadLeaderboardEntries::Fail(const FString& Why)
{
	OnFailure.Broadcast(Why);
	SetReadyToDestroy();
}
