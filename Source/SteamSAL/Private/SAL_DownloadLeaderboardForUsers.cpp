// Copyright (c) 2025 UnForge. All rights reserved.

#include "SAL_DownloadLeaderboardForUsers.h"
#include "SAL_Internal.h"

USAL_DownloadLeaderboardForUsers* USAL_DownloadLeaderboardForUsers::DownloadEntriesForUsers(
	UObject* WorldContextObject, FSAL_LeaderboardHandle LeaderboardHandle,
	const TArray<FString>& SteamIDs)
{
	USAL_DownloadLeaderboardForUsers* Node = NewObject<USAL_DownloadLeaderboardForUsers>();

	Node->RegisterWithGameInstance(WorldContextObject);

	Node->WorldContextObject     = WorldContextObject;
	Node->InHandle               = LeaderboardHandle;
	Node->InSteamId64Strings     = SteamIDs;

	return Node;
}

void USAL_DownloadLeaderboardForUsers::Activate()
{
	if (InHandle.Value == 0)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: Invalid LeaderboardHandle"));
		SetReadyToDestroy();
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: SteamUserStats not available"));
		SetReadyToDestroy();
		return;
	}

	if (InSteamId64Strings.Num() <= 0)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: Empty user list"));
		SetReadyToDestroy();
		return;
	}

	constexpr int32 kMaxUsers = 100;
	const int32 CountClamped = FMath::Min(InSteamId64Strings.Num(), kMaxUsers);

	TArray<CSteamID> SteamIDs;
	SteamIDs.Reserve(CountClamped);

	for (int32 i = 0; i < CountClamped; ++i)
	{
		const FString& S = InSteamId64Strings[i];

		uint64 Raw64 = 0;
		if (!LexTryParseString<uint64>(Raw64, *S))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadEntriesForUsers: Bad SteamID64 string '%s' (skipped)"), *S);
			continue;
		}

		const CSteamID Cid((uint64)Raw64);
		if (!Cid.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadEntriesForUsers: Invalid SteamID64 '%s' (skipped)"), *S);
			continue;
		}

		SteamIDs.Add(Cid);
	}

	if (SteamIDs.Num() <= 0)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: No valid Steam IDs after parsing"));
		SetReadyToDestroy();
		return;
	}

	SteamAPICall_t ApiCall = SteamUserStats()->DownloadLeaderboardEntriesForUsers(
		(SteamLeaderboard_t)InHandle.Value,
		SteamIDs.GetData(),
		SteamIDs.Num()
	);

	if (ApiCall == k_uAPICallInvalid)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: DownloadLeaderboardEntriesForUsers returned invalid call handle"));
		SetReadyToDestroy();
		return;
	}

	DownloadCallResult.Set(
		ApiCall,
		this,
		&USAL_DownloadLeaderboardForUsers::OnScoresDownloaded
	);
}

void USAL_DownloadLeaderboardForUsers::OnScoresDownloaded(LeaderboardScoresDownloaded_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: IO failure or null callback"));
		SetReadyToDestroy();
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		OnFailure.Broadcast(TEXT("[SAL] DownloadEntriesForUsers: SteamUserStats unavailable in callback"));
		SetReadyToDestroy();
		return;
	}

	const SteamLeaderboard_t Expected = static_cast<SteamLeaderboard_t>(InHandle.Value);
	if (Callback->m_hSteamLeaderboard != Expected)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] DownloadEntriesForUsers: Callback for a different leaderboard handle"));
	}

	FSAL_LeaderboardEntriesData EntriesData;
	EntriesData.RequestType     = ELeaderboardRequestType::Global;
	EntriesData.TotalEntryCount = Callback->m_cEntryCount;

	if (Callback->m_cEntryCount > 0)
	{
		EntriesData.Entries.Reserve(Callback->m_cEntryCount);
	}

	if (Callback->m_cEntryCount <= 0)
	{
		TWeakObjectPtr<USAL_DownloadLeaderboardForUsers> Self(this);

		SAL_RunOnGameThread([Self, EntriesData]()
		{
			if (!Self.IsValid()) return;
			Self->OnSuccess.Broadcast(EntriesData, 0);
			Self->SetReadyToDestroy();
		});

		return;
	}

	LeaderboardEntry_t RawEntry;
	int32 DetailBuffer[64];

	for (int32 i = 0; i < Callback->m_cEntryCount; ++i)
	{
		if (SteamUserStats()->GetDownloadedLeaderboardEntry(
			Callback->m_hSteamLeaderboardEntries,
			i,
			&RawEntry,
			DetailBuffer,
			UE_ARRAY_COUNT(DetailBuffer)))
		{
			FSAL_LeaderboardEntryRow Row;
			Row.SteamID    = FString::Printf(TEXT("%llu"), RawEntry.m_steamIDUser.ConvertToUint64());
			Row.GlobalRank = RawEntry.m_nGlobalRank;
			Row.Score      = RawEntry.m_nScore;

			if (SteamFriends())
			{
				const char* Persona = SteamFriends()->GetFriendPersonaName(RawEntry.m_steamIDUser);
				Row.PlayerName = Persona ? UTF8_TO_TCHAR(Persona) : TEXT("");

				if (Row.PlayerName.IsEmpty())
				{
					SteamFriends()->RequestUserInformation(RawEntry.m_steamIDUser, true);
				}
			}

			for (int32 d = 0; d < RawEntry.m_cDetails; ++d)
			{
				Row.Details.Add(DetailBuffer[d]);
			}

			EntriesData.Entries.Add(Row);
		}
	}

	const int32 EntryCount = EntriesData.Entries.Num();

	TWeakObjectPtr<USAL_DownloadLeaderboardForUsers> Self(this);

	SAL_RunOnGameThread([Self, EntriesData, EntryCount]() mutable
	{
		if (!Self.IsValid()) return;
		Self->OnSuccess.Broadcast(EntriesData, EntryCount);
		Self->SetReadyToDestroy();
	});
}
