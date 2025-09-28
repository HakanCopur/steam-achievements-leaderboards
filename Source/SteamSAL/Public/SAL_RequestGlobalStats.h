// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_RequestGlobalStats.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSAL_RequestGlobalStatsSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSAL_RequestGlobalStatsFailure);

UCLASS()
class STEAMSAL_API USAL_RequestGlobalStats : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Stats",
		meta=(BlueprintInternalUseOnly="true",
			  DisplayName="Request Global Stats",
			  ToolTip="Requests global aggregated stats from Steam for N days (0 = all-time). Must complete successfully before reading global stats/history.",
			  Keywords="steam stats global aggregated request fetch cache history"
	))
	static USAL_RequestGlobalStats* RequestGlobalStats(int32 Days = 0);

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Stats")
	FSAL_RequestGlobalStatsSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Stats")
	FSAL_RequestGlobalStatsFailure OnFailure;

	virtual void Activate() override;

private:
	int32 RequestedDays = 0;

	CCallResult<USAL_RequestGlobalStats, GlobalStatsReceived_t> CallResult;

	void OnGlobalStatsReceived(GlobalStatsReceived_t* Callback, bool bIOFailure);

};
