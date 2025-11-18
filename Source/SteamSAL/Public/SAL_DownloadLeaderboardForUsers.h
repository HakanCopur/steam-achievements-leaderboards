// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_DownloadLeaderboardForUsers.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSAL_OnDownloadEntriesForUsersSuccess,
	const FSAL_LeaderboardEntriesData&, EntriesData,
	int32, EntryCount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSAL_OnDownloadEntriesForUsersFailure,
	const FString&, Error
);

UCLASS()
class STEAMSAL_API USAL_DownloadLeaderboardForUsers : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable,
		Category="SteamSAL|Leaderboard",
		meta=(
			BlueprintInternalUseOnly="true",
			WorldContext="WorldContextObject",
			DisplayName="Download Steam Leaderboard Entries (Users)",
			Keywords="Steam Leaderboard Download Entries Users Friends Ranks Scores IDs Player"
		))
	static USAL_DownloadLeaderboardForUsers* DownloadEntriesForUsers(
		UObject* WorldContextObject,
		FSAL_LeaderboardHandle LeaderboardHandle,
		const TArray<FString>& SteamIDs
	);

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard",
		meta=(ToolTip="Fires with the downloaded entries. EntryCount will be 0 if users have no entries on this leaderboard."))
	FSAL_OnDownloadEntriesForUsersSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard",
		meta=(ToolTip="Fires if the request fails. Contains a human-readable reason."))
	FSAL_OnDownloadEntriesForUsersFailure OnFailure;

	virtual void Activate() override;

private:
	UPROPERTY()
	UObject* WorldContextObject = nullptr;

	UPROPERTY()
	FSAL_LeaderboardHandle InHandle;

	UPROPERTY()
	TArray<FString> InSteamId64Strings;

	CCallResult<USAL_DownloadLeaderboardForUsers, LeaderboardScoresDownloaded_t> DownloadCallResult;

	void OnScoresDownloaded(LeaderboardScoresDownloaded_t* Callback, bool bIOFailure);
};
