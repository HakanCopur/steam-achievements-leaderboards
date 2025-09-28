// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_FindLeaderboard.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_FindLeaderboardSuccess, FSAL_LeaderboardHandle, LeaderboardHandle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_FindLeaderboardFailure, FString, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_FindLeaderboard : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public :
	UFUNCTION(BlueprintCallable, Category = "SteamSAL|Leaderboards",
		meta=(WorldContext="WorldContextObject", BlueprintInternalUseOnly="true", DisplayName="Find Steam Leaderboard",
			ToolTip=
			"Finds an existing Steam leaderboard by name and returns a valid handle. Does not create new leaderboards.",
			Keywords="steam leaderboard find search"
		))
	static USAL_FindLeaderboard* FindLeaderboard(
		const UObject* WorldContextObject,
		UPARAM(meta=(ToolTip="Exact leaderboard name to search on Steam (case-sensitive)."))
		const FString& LeaderboardName);

	UPROPERTY(BlueprintAssignable)
	FSAL_FindLeaderboardSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable)
	FSAL_FindLeaderboardFailure OnFailure;
	
	virtual void Activate() override;

private:
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	FString InLeaderboardName;
	CCallResult<USAL_FindLeaderboard, LeaderboardFindResult_t> FindLeaderboardCallResult;
	
	void OnFindLeaderboardCompleted(LeaderboardFindResult_t* Result, bool bIOFailure);

};
