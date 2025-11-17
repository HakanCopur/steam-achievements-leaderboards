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
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: UGCFileName is empty."));
		return;
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

	// 1) Write the file to Steam Remote Storage (synchronous)
	const FTCHARToUTF8 Utf8FileName(*InUGCFileName);

	const bool bWriteOk = SteamRemoteStorage()->FileWrite(
		Utf8FileName.Get(),
		InUGCData.GetData(),
		InUGCData.Num()
	);

	if (!bWriteOk)
	{
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: FileWrite to Remote Storage failed."));
		return;
	}

	// 2) Start asynchronous FileShare to get a UGC handle
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

	// Store the shared UGC handle for later attach
	SharedUGCHandle.Value = static_cast<int64>(Callback->m_hFile);

	// 3) Now upload the leaderboard score
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
		k_ELeaderboardUploadScoreMethodKeepBest, // same default behavior as typical upload
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

	// Keep the final score Steam reports (in case it changed).
	InScore = Callback->m_nScore;

	// 4) Attach the shared UGC handle to the uploaded score
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
		Fail(TEXT("[SteamSAL] UploadScoreWithUGC: AttachLeaderboardUGC IO failure."));
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

	TWeakObjectPtr<USAL_UploadScoreWithUGC> Self(this);

	SAL_RunOnGameThread([Self, FinalScore, FinalHandle]()
	{
		if (!Self.IsValid()) return;

		Self->OnSuccess.Broadcast(
			FinalScore,
			FinalHandle,
			TEXT("[SteamSAL] UploadScoreWithUGC: Score and UGC successfully uploaded and attached.")
		);
		Self->SetReadyToDestroy();
	});
}

void USAL_UploadScoreWithUGC::Fail(const FString& Why)
{
	const FString WhyCopy = Why;
	TWeakObjectPtr<USAL_UploadScoreWithUGC> Self(this);

	SAL_RunOnGameThread([Self, WhyCopy]()
	{
		if (!Self.IsValid()) return;

		Self->OnFailure.Broadcast(WhyCopy);
		Self->SetReadyToDestroy();
	});
}
