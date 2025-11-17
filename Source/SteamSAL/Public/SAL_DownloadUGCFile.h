// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_DownloadUGCFile.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSAL_OnDownloadUGCFileSuccess,
                                              const FSAL_UGCHandle&, UGCHandle,
                                              const TArray<uint8>&, Data,
                                              int32, DataSize,
                                              const FString&, DebugMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnDownloadUGCFileFailure,
                                            const FString&, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_DownloadUGCFile : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|UGC",
		meta=(WorldContext="WorldContextObject",
			BlueprintInternalUseOnly="true",
			ToolTip=
			"Downloads the raw bytes of a UGC file from Steam Remote Storage using a UGC handle.\n Use this together with Downloaded leaderboard entries (UGCHandle) or UploadScoreWithUGC."
			,
			Keywords="steam ugc download file remote storage cloud"),
		DisplayName="Download Steam UGC File")
	static USAL_DownloadUGCFile* DownloadUGCFile(
		UObject* WorldContextObject,
		UPARAM(meta=(ToolTip="A valid UGC handle, for example from a leaderboard entry or UploadScoreWithUGC."))
		FSAL_UGCHandle UGCHandle,
		UPARAM(meta=(ToolTip="Optional hard limit for bytes to download. 0 or negative = no explicit limit."))
		int32 MaxBytes = 0
	);

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|UGC")
	FSAL_OnDownloadUGCFileSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="SteamSAL|UGC")
	FSAL_OnDownloadUGCFileFailure OnFailure;

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;

private:
	UPROPERTY()
	UObject* WorldContextObject = nullptr;

	FSAL_UGCHandle InUGCHandle;
	int32 InMaxBytes = 0;

	TArray<uint8> DownloadedData;

	CCallResult<USAL_DownloadUGCFile, RemoteStorageDownloadUGCResult_t> DownloadCallResult;

	void StartDownload();

	void OnDownloadCompleted(RemoteStorageDownloadUGCResult_t* Callback, bool bIOFailure);

	void Fail(const FString& Why);
};
