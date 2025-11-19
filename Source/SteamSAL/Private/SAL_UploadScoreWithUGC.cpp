// Copyright (c) 2025 UnForge. All rights reserved.

#include "SAL_UploadScoreWithUGC.h"
#include "SAL_Internal.h"

USAL_UploadScoreWithUGC* USAL_UploadScoreWithUGC::UploadScoreWithUGC(
	UObject* WorldContextObject,
	FSAL_LeaderboardHandle LeaderboardHandle,
	int32 Score,
	TArray<int32> Details,
	FString UGCFileName,
	TArray<uint8> UGCData)
{
	USAL_UploadScoreWithUGC* Node = NewObject<USAL_UploadScoreWithUGC>();

	if (Node)
	{
		Node->RegisterWithGameInstance(WorldContextObject);

		Node->WorldContextObject = WorldContextObject;
		Node->InHandle           = LeaderboardHandle;
		Node->InScore            = Score;
		Node->InDetails          = Details;
		Node->InUGCFileName      = UGCFileName;
		Node->InUGCData          = UGCData;
	}

	return Node;
}

void USAL_UploadScoreWithUGC::Activate()
{
	if (InHandle.Value == 0)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: Invalid leaderboard handle."));
		return;
	}

	if (InUGCFileName.IsEmpty())
	{
		const FString AutoFileName = FString::Printf(
			TEXT("SteamSAL_UGC_%llu.json"),
			(uint64)FDateTime::UtcNow().ToUnixTimestamp()
		);

		UE_LOG(LogTemp, Warning,
			   TEXT("[SteamSAL] UploadScoreWithUGC: No UGCFileName provided. Using auto-generated '%s'"),
			   *AutoFileName);

		InUGCFileName = AutoFileName;
	}

	if (InUGCData.Num() <= 0)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: UGCData is empty."));
		return;
	}

	if (SteamRemoteStorage() == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: SteamRemoteStorage is not available."));
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: SteamUserStats is not available."));
		return;
	}
	
	const FTCHARToUTF8 Utf8FileName(*InUGCFileName);
	
	const bool bWriteOk = SteamRemoteStorage()->FileWrite(
	Utf8FileName.Get(),
	InUGCData.GetData(),
	InUGCData.Num()
);
	
	const bool bCloudAccount = SteamRemoteStorage()->IsCloudEnabledForAccount();
	const bool bCloudApp     = SteamRemoteStorage()->IsCloudEnabledForApp();

	AppId_t AppId = k_uAppIdInvalid;
	if (SteamUtils())
	{
		AppId = SteamUtils()->GetAppID();
	}

	UE_LOG(LogTemp, Warning,
		   TEXT("[SteamSAL] UploadScoreWithUGC: Cloud status - Account=%s, App=%s, AppID=%u, FileName='%s', Size=%d"),
		   bCloudAccount ? TEXT("ENABLED") : TEXT("DISABLED"),
		   bCloudApp     ? TEXT("ENABLED") : TEXT("DISABLED"),
		   (uint32)AppId,
		   *InUGCFileName,
		   InUGCData.Num());
	
	uint64 TotalBytes = 0;
	uint64 AvailableBytes = 0;
	const bool bGotQuota = SteamRemoteStorage()->GetQuota(&TotalBytes, &AvailableBytes);

	UE_LOG(LogTemp, Warning,
		   TEXT("[SteamSAL] UploadScoreWithUGC: Quota - GotQuota=%s, Total=%llu, Available=%llu"),
		   bGotQuota ? TEXT("true") : TEXT("false"),
		   (unsigned long long)TotalBytes,
		   (unsigned long long)AvailableBytes);
	
	if (!bWriteOk)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: FileWrite to Remote Storage failed."));
		return;
	}
	
	StartFileShare();
}

void USAL_UploadScoreWithUGC::StartFileShare()
{
	if (SteamRemoteStorage() == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: SteamRemoteStorage is not available for FileShare."));
		return;
	}

	const FTCHARToUTF8 Utf8FileName(*InUGCFileName);

	SteamAPICall_t ApiCall = SteamRemoteStorage()->FileShare(Utf8FileName.Get());
	if (ApiCall == k_uAPICallInvalid)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: FileShare returned an invalid API call handle."));
		return;
	}

	FileShareCallResult.Set(ApiCall, this, &USAL_UploadScoreWithUGC::OnFileShared);
}

void USAL_UploadScoreWithUGC::OnFileShared(RemoteStorageFileShareResult_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: FileShare IO failure."));
		return;
	}

	if (Callback->m_eResult != k_EResultOK)
	{
		const FString Reason = FString::Printf(
			TEXT("[SteamSAL] UploadScoreWithUGC: FileShare failed with result %d."),
			static_cast<int32>(Callback->m_eResult)
		);
		Fail(Reason);
		return;
	}
	
	SharedUGCHandle.Value = static_cast<int64>(Callback->m_hFile);
	StartUploadScore();
}

void USAL_UploadScoreWithUGC::StartUploadScore()
{
	if (SteamUserStats() == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: SteamUserStats is not available for UploadLeaderboardScore."));
		return;
	}

	const SteamLeaderboard_t SteamHandle = static_cast<SteamLeaderboard_t>(InHandle.Value);

	const int32* DetailsPtr = nullptr;
	int32        DetailsCount = 0;

	if (InDetails.Num() > 0)
	{
		DetailsPtr   = InDetails.GetData();
		DetailsCount = InDetails.Num();
	}

	SteamAPICall_t ApiCall = SteamUserStats()->UploadLeaderboardScore(
		SteamHandle,
		k_ELeaderboardUploadScoreMethodKeepBest,
		InScore,
		DetailsPtr,
		DetailsCount
	);

	if (ApiCall == k_uAPICallInvalid)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: UploadLeaderboardScore returned an invalid API call handle."));
		return;
	}

	ScoreUploadedCallResult.Set(ApiCall, this, &USAL_UploadScoreWithUGC::OnScoreUploaded);
}

void USAL_UploadScoreWithUGC::OnScoreUploaded(LeaderboardScoreUploaded_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: UploadLeaderboardScore IO failure."));
		return;
	}

	if (!Callback->m_bSuccess)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: UploadLeaderboardScore reported failure (m_bSuccess == false)."));
		return;
	}

	InScore = Callback->m_nScore;

	StartAttachUGC();
}

void USAL_UploadScoreWithUGC::StartAttachUGC()
{
	if (!SharedUGCHandle.IsValid())
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: Shared UGC handle is invalid, cannot attach."));
		return;
	}

	if (SteamUserStats() == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: SteamUserStats is not available for AttachLeaderboardUGC."));
		return;
	}

	const SteamLeaderboard_t SteamHandle   = static_cast<SteamLeaderboard_t>(InHandle.Value);
	const UGCHandle_t        SteamUGCHandle = static_cast<UGCHandle_t>(SharedUGCHandle.Value);

	SteamAPICall_t ApiCall = SteamUserStats()->AttachLeaderboardUGC(SteamHandle, SteamUGCHandle);
	if (ApiCall == k_uAPICallInvalid)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: AttachLeaderboardUGC returned an invalid API call handle."));
		return;
	}

	AttachUGCCallResult.Set(ApiCall, this, &USAL_UploadScoreWithUGC::OnUGCAttached);
}

void USAL_UploadScoreWithUGC::OnUGCAttached(LeaderboardUGCSet_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: AttachLeaderboardUGC IO failure or null callback."));
		return;
	}

	if (Callback->m_eResult != k_EResultOK)
	{
		const FString Reason = FString::Printf(
			TEXT("[SteamSAL] UploadScoreWithUGC: AttachLeaderboardUGC failed with result %d."),
			static_cast<int32>(Callback->m_eResult)
		);

		Fail(Reason);
		return;
	}

	const int32          FinalScore  = InScore;
	const FSAL_UGCHandle FinalHandle = SharedUGCHandle;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[SteamSAL] UploadScoreWithUGC: AttachLeaderboardUGC success (Score=%d, UGCHandle=%lld)."),
		FinalScore,
		static_cast<long long>(FinalHandle.Value)
	);

	OnSuccess.Broadcast(
		FinalScore,
		FinalHandle
	);

	SetReadyToDestroy();
}


void USAL_UploadScoreWithUGC::Fail(const FString& Why)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Why);

	OnFailure.Broadcast(Why);
	SetReadyToDestroy();
}

