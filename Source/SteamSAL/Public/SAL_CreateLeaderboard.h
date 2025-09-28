// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_CreateLeaderboard.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_CreateLeaderboardSuccess, FSAL_LeaderboardHandle, LeaderboardHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_CreateLeaderboardFailure, FString, ErrorMessage);


UCLASS()
class STEAMSAL_API USAL_CreateLeaderboard : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Leaderboards",
		DisplayName="Create Steam Leaderboard",
		meta=(WorldContext="WorldContextObject",
			BlueprintInternalUseOnly="true",
			ToolTip="Creates the leaderboard if missing; returns a handle.",
			Keywords="steam leaderboard create make new"))
	static USAL_CreateLeaderboard* CreateLeaderboard(
		const UObject* WorldContextObject,
		UPARAM(meta=(ToolTip="Exact leaderboard name as it will appear on Steam."))
		const FString& LeaderboardName,
		UPARAM(DisplayName="Sort Method",
			meta=(ToolTip="Ascending: lower is better (time). Descending: higher is better (points)."))
		ESALLeaderboardSortMethod SortMethod = ESALLeaderboardSortMethod::Descending,
		UPARAM(DisplayName="Display Type",
			meta=(ToolTip="How Steam presents the score (numeric/time)."))
		ESALLeaderboardDisplayType DisplayType = ESALLeaderboardDisplayType::Numeric);

	UPROPERTY(BlueprintAssignable)
	FSAL_CreateLeaderboardSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable)
	FSAL_CreateLeaderboardFailure OnFailure;


	virtual void Activate() override;

private:
	UPROPERTY()
	UObject* WorldContextObject = nullptr;

	FString InLeaderboardName{};
	ESALLeaderboardSortMethod InSortMethod = ESALLeaderboardSortMethod::Descending;
	ESALLeaderboardDisplayType InDisplayType = ESALLeaderboardDisplayType::Numeric;

	CCallResult<USAL_CreateLeaderboard, LeaderboardFindResult_t> CreateLeaderboardCallResult;

	void OnCreateLeaderboardCompleted(LeaderboardFindResult_t* Result, bool bIOFailure);
};
