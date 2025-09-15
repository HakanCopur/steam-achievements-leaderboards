// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_DownloadLeaderboardEntries.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnLeaderboardEntriesSuccess, const TArray<FSAL_LeaderboardEntryRow>&,
                                            Entries);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnLeaderboardEntriesFailure, const FString&, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_DownloadLeaderboardEntries : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Factory function (we'll add the signature next step)
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Leaderboard",
		meta=(WorldContext="WorldContextObject",
			BlueprintInternalUseOnly="true",
			ToolTip=
			"Download leaderboard entries by range or request type (Global, Around User, Friends).\nUse RangeStart and RangeEnd for the range of results, or 0,0 for Friends.",
			AdvancedDisplay="DetailsMax",
			Keywords="steam leaderboard download entries range around user friends scores ranks"),
		DisplayName="Get Steam Leaderboard Entries")
	static USAL_DownloadLeaderboardEntries* DownloadLeaderboardEntries(
		const UObject* WorldContextObject,
		UPARAM(meta=(ToolTip="Valid leaderboard handle obtained from FindLeaderboard"))
		FSAL_LeaderboardHandle LeaderboardHandle,
		UPARAM(
			meta=(ToolTip=
				"Which entries to request:\n- Global: absolute ranks\n- AroundUser: relative to local player\n- Friends: all Steam friends"
			))
		ELeaderboardRequestType RequestType,
		UPARAM(
			meta=(ToolTip=
				"Start index for entries.\nFor Global: 1 = top rank.\nFor AroundUser: negative = entries before user."))
		int32 RangeStart,
		UPARAM(
			meta=(ToolTip=
				"End index for entries (inclusive).\nFor Global: higher = more entries.\nFor AroundUser: positive = entries after user."
			))
		int32 RangeEnd,
		UPARAM(meta=(ToolTip="Maximum number of 'details' integers per entry.\nUse 0 if you don't need extra details."))
		int32 DetailsMax = 0
	);
	
	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard")
	FSAL_OnLeaderboardEntriesSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard")
	FSAL_OnLeaderboardEntriesFailure OnFailure;

	virtual void Activate() override;

private:
	UPROPERTY()
	UObject* WorldContextObject = nullptr;

	FSAL_LeaderboardHandle InHandle{};
	ELeaderboardRequestType InRequestType = ELeaderboardRequestType::Global;
	int32 InRangeStart = 1;
	int32 InRangeEnd = 10;
	int32 InDetailsMax = 0;

	CCallResult<USAL_DownloadLeaderboardEntries, LeaderboardScoresDownloaded_t> CallResult;

	void OnScoresDownloaded(LeaderboardScoresDownloaded_t* Callback, bool bIOFailure);
	void Fail(const FString& Why);
	
};
