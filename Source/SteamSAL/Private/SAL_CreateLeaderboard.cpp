// Copyright (c) 2025 UnForge. All rights reserved.


#include "SAL_CreateLeaderboard.h"

USAL_CreateLeaderboard* USAL_CreateLeaderboard::CreateLeaderboard(const UObject* WorldContextObject,
	const FString& LeaderboardName, ESALLeaderboardSortMethod SortMethod, ESALLeaderboardDisplayType DisplayType)
{
	USAL_CreateLeaderboard* Node = NewObject<USAL_CreateLeaderboard>();
	
	Node->RegisterWithGameInstance(WorldContextObject);
	
	Node->InLeaderboardName = LeaderboardName;
	Node->InSortMethod = SortMethod;
	Node->InDisplayType = DisplayType;
	Node->WorldContextObject = const_cast<UObject*>(WorldContextObject);
	
	return Node;
}

void USAL_CreateLeaderboard::Activate()
{
	if (InLeaderboardName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] CreateLeaderboard: LeaderboardName is empty."));
		OnFailure.Broadcast(TEXT("CreateLeaderboard: LeaderboardName is empty."));
		SetReadyToDestroy();
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] CreateLeaderboard: Steam not initialized (SteamUserStats == nullptr)."));
		OnFailure.Broadcast(TEXT("CreateLeaderboard: Steam not initialized (SteamUserStats == nullptr)."));
		SetReadyToDestroy();
		return;
	}

	ANSICHAR NameAnsi[512];
	FCStringAnsi::Strncpy(NameAnsi, TCHAR_TO_ANSI(*InLeaderboardName), UE_ARRAY_COUNT(NameAnsi));

	ELeaderboardSortMethod SteamSort = k_ELeaderboardSortMethodDescending;
	switch (InSortMethod)
	{
	case ESALLeaderboardSortMethod::Ascending:  SteamSort = k_ELeaderboardSortMethodAscending;  break;
	case ESALLeaderboardSortMethod::Descending: SteamSort = k_ELeaderboardSortMethodDescending; break;
	default: break;
	}

	ELeaderboardDisplayType SteamDisplay = k_ELeaderboardDisplayTypeNumeric;
	switch (InDisplayType)
	{
	case ESALLeaderboardDisplayType::Numeric:          SteamDisplay = k_ELeaderboardDisplayTypeNumeric;          break;
	case ESALLeaderboardDisplayType::TimeSeconds:      SteamDisplay = k_ELeaderboardDisplayTypeTimeSeconds;      break;
	case ESALLeaderboardDisplayType::TimeMilliSeconds: SteamDisplay = k_ELeaderboardDisplayTypeTimeMilliSeconds; break;
	default: break;
	}

	SteamAPICall_t Call = SteamUserStats()->FindOrCreateLeaderboard(NameAnsi, SteamSort, SteamDisplay);
	if (Call == k_uAPICallInvalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] CreateLeaderboard: Steam returned invalid APICall"));
		OnFailure.Broadcast(TEXT("Steam returned an invalid API call handle."));
		SetReadyToDestroy();
		return;
	}
	CreateLeaderboardCallResult.Set(Call, this, &USAL_CreateLeaderboard::OnCreateLeaderboardCompleted);

	UE_LOG(LogTemp, Log, TEXT("[SAL] CreateLeaderboard requested: '%s' (Sort=%d, Display=%d)"),
		*InLeaderboardName, static_cast<int32>(InSortMethod), static_cast<int32>(InDisplayType));
}

void USAL_CreateLeaderboard::OnCreateLeaderboardCompleted(LeaderboardFindResult_t* Result, bool bIOFailure)
{
	if (bIOFailure || Result == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[SAL] CreateLeaderboard: IO failure or null result"));
		OnFailure.Broadcast(TEXT("Steam IO failure during CreateLeaderboard."));
		SetReadyToDestroy();
		return;
	}

	if (!Result->m_bLeaderboardFound || Result->m_hSteamLeaderboard == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] CreateLeaderboard: '%s' not found/created"), *InLeaderboardName);
		OnFailure.Broadcast(FString::Printf(TEXT("Leaderboard '%s' not found/created."), *InLeaderboardName));
		SetReadyToDestroy();
		return;
	}

	FSAL_LeaderboardHandle Handle;
	Handle.Value = static_cast<int64>(Result->m_hSteamLeaderboard);

	UE_LOG(LogTemp, Log, TEXT("[SAL] CreateLeaderboard: success '%s' (Handle=%lld)"),
		*InLeaderboardName, Handle.Value);

	OnSuccess.Broadcast(Handle);
	SetReadyToDestroy();
}

