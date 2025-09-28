// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#include "SAL_UploadScore.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSAL_UploadScoreSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_UploadScoreFailure, FString, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_UploadScore : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Leaderboard",
		DisplayName="Upload Steam Leaderboard Score",
		meta=(WorldContext="WorldContextObject",
			BlueprintInternalUseOnly="true",
			AdvancedDisplay="Details",
			AutoCreateRefTerm="Details",
			ToolTip="Uploads a score with optional Details (up to 64 ints)."))
	static USAL_UploadScore* UploadScore(
		const UObject* WorldContextObject,
		FSAL_LeaderboardHandle LeaderboardHandle,
		int32 Score,
		ESALLeaderboardUploadMethod UploadMethod,
		const TArray<int32>& Details);

	UPROPERTY(BlueprintAssignable)
	FSAL_UploadScoreSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable)
	FSAL_UploadScoreFailure OnFailure;

private:
	virtual void Activate() override;

	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	FSAL_LeaderboardHandle LeaderboardHandle;
	int32 Score;
	ESALLeaderboardUploadMethod UploadMethod;
	TArray<int32> InDetails;

	CCallResult<USAL_UploadScore, LeaderboardScoreUploaded_t> UploadCallResult;
	void OnUploadCompleted(LeaderboardScoreUploaded_t* Result, bool bIOFailure);
};
