// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_UploadScoreWithUGC.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSAL_OnUploadScoreWithUGCSuccess,
                                               int32, NewScore,
                                               const FSAL_UGCHandle&, UGCHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnUploadScoreWithUGCFailure,
                                            const FString&, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_UploadScoreWithUGC : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Leaderboard|UGC",
		meta=(WorldContext="WorldContextObject",
			BlueprintInternalUseOnly="true",
			ToolTip=
			"Upload a score to a Steam leaderboard and attach a UGC file in a single call.\n The node will:\n- Write the UGC data to Steam Remote Storage\n- Share the file to obtain a UGC handle\n- Upload the score\n- Attach the UGC handle to the uploaded score."
			,
			AutoCreateRefTerm = "Details,UGCData",
			Keywords="steam leaderboard upload score ugc file remote storage attach"),
		DisplayName="Upload Steam Leaderboard Score With UGC")
	static USAL_UploadScoreWithUGC* UploadScoreWithUGC(
		UObject* WorldContextObject,
		UPARAM(meta=(ToolTip="Valid leaderboard handle obtained from FindLeaderboard"))
		FSAL_LeaderboardHandle LeaderboardHandle,
		UPARAM(meta=(ToolTip="Score value to upload to this leaderboard."))
		int32 Score,
		UPARAM(meta=(ToolTip="Optional per-score metadata, same as in UploadLeaderboardScore. Can be empty."))
		TArray<int32> Details,
		UPARAM(meta=(ToolTip="File name to use in Steam Remote Storage for this UGC payload (no path)."))
		FString UGCFileName,
		UPARAM(
			meta=(ToolTip=
				"Binary contents to write into the UGC file. Use a separate conversion node if you want to send text/JSON."
			))
		TArray<uint8> UGCData
	);

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard|UGC")
	FSAL_OnUploadScoreWithUGCSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|Leaderboard|UGC")
	FSAL_OnUploadScoreWithUGCFailure OnFailure;

	virtual void Activate() override;

private:
	UPROPERTY()
	UObject* WorldContextObject = nullptr;

	FSAL_LeaderboardHandle InHandle{};
	int32 InScore = 0;
	TArray<int32> InDetails;
	FString InUGCFileName;
	TArray<uint8> InUGCData;

	FSAL_UGCHandle SharedUGCHandle;

	CCallResult<USAL_UploadScoreWithUGC, RemoteStorageFileShareResult_t> FileShareCallResult;
	CCallResult<USAL_UploadScoreWithUGC, LeaderboardScoreUploaded_t> ScoreUploadedCallResult;
	CCallResult<USAL_UploadScoreWithUGC, LeaderboardUGCSet_t> AttachUGCCallResult;

	void StartFileShare();
	void StartUploadScore();
	void StartAttachUGC();

	void OnFileShared(RemoteStorageFileShareResult_t* Callback, bool bIOFailure);
	void OnScoreUploaded(LeaderboardScoreUploaded_t* Callback, bool bIOFailure);
	void OnUGCAttached(LeaderboardUGCSet_t* Callback, bool bIOFailure);

	void Fail(const FString& Why);
};
