// Copyright (c) 2025 UnForge. All rights reserved.

#include "SAL_DownloadUGCFile.h"
#include "SAL_Internal.h"
#include "Misc/EngineVersionComparison.h"

USAL_DownloadUGCFile* USAL_DownloadUGCFile::DownloadUGCFile(
	UObject* WorldContextObject,
	FSAL_UGCHandle UGCHandle,
	int32 MaxBytes)
{
	USAL_DownloadUGCFile* Node = NewObject<USAL_DownloadUGCFile>();

	if (Node)
	{
		Node->RegisterWithGameInstance(WorldContextObject);

		Node->WorldContextObject = WorldContextObject;
		Node->InUGCHandle        = UGCHandle;
		Node->InMaxBytes         = MaxBytes;
	}

	return Node;
}

void USAL_DownloadUGCFile::Activate()
{
	if (!InUGCHandle.IsValid())
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: Invalid UGC handle."));
		return;
	}

	if (SteamRemoteStorage() == nullptr)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: SteamRemoteStorage is not available."));
		return;
	}

	StartDownload();
}

void USAL_DownloadUGCFile::StartDownload()
{
	if (SteamRemoteStorage() == nullptr)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: SteamRemoteStorage is not available in StartDownload."));
		return;
	}

	const UGCHandle_t SteamUGCHandle = static_cast<UGCHandle_t>(InUGCHandle.Value);

	// Priority 0 is fine for normal use.
	SteamAPICall_t ApiCall = SteamRemoteStorage()->UGCDownload(SteamUGCHandle, 0);
	if (ApiCall == k_uAPICallInvalid)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: UGCDownload returned an invalid API call handle."));
		return;
	}

	DownloadCallResult.Set(ApiCall, this, &USAL_DownloadUGCFile::OnDownloadCompleted);
}

void USAL_DownloadUGCFile::OnDownloadCompleted(RemoteStorageDownloadUGCResult_t* Callback, bool bIOFailure)
{
	if (bIOFailure || Callback == nullptr)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: UGCDownload IO failure."));
		return;
	}

	if (Callback->m_eResult != k_EResultOK)
	{
		const FString Reason = FString::Printf(
			TEXT("[SteamSAL] DownloadUGCFile: UGCDownload failed with result %d."),
			static_cast<int32>(Callback->m_eResult)
		);
		Fail(Reason);
		return;
	}

	if (SteamRemoteStorage() == nullptr)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: SteamRemoteStorage not available in OnDownloadCompleted."));
		return;
	}

	const int32 TotalSize = static_cast<int32>(Callback->m_nSizeInBytes);
	if (TotalSize <= 0)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: UGC file size is zero or negative."));
		return;
	}

	int32 BytesToRead = TotalSize;
	if (InMaxBytes > 0 && InMaxBytes < BytesToRead)
	{
		BytesToRead = InMaxBytes;
	}

	DownloadedData.SetNumUninitialized(BytesToRead);

	const int32 BytesRead = SteamRemoteStorage()->UGCRead(
		Callback->m_hFile,
		DownloadedData.GetData(),
		BytesToRead,
		0,
		k_EUGCRead_Close
	);

	if (BytesRead < 0)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: UGCRead failed (negative byte count)."));
		return;
	}

	if (BytesRead == 0)
	{
		Fail(TEXT("[SteamSAL] DownloadUGCFile: UGCRead returned 0 bytes."));
		return;
	}

	if (BytesRead < BytesToRead)
	{
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		DownloadedData.SetNum(BytesRead, false);
#else
		DownloadedData.SetNum(BytesRead, EAllowShrinking::No);
#endif
		
	}

	const int32 FinalSize = DownloadedData.Num();
	TArray<uint8> DataCopy = MoveTemp(DownloadedData);
	const FSAL_UGCHandle HandleCopy = InUGCHandle;

	TWeakObjectPtr<USAL_DownloadUGCFile> Self(this);

	SAL_RunOnGameThread([Self, DataCopy = MoveTemp(DataCopy), FinalSize, HandleCopy]() mutable
	{
		if (!Self.IsValid()) return;

		Self->OnSuccess.Broadcast(
			HandleCopy,
			DataCopy,
			FinalSize
		);

		Self->SetReadyToDestroy();
	});
}

void USAL_DownloadUGCFile::Fail(const FString& Why)
{
	const FString WhyCopy = Why;
	TWeakObjectPtr<USAL_DownloadUGCFile> Self(this);

	SAL_RunOnGameThread([Self, WhyCopy]()
	{
		if (!Self.IsValid()) return;

		Self->OnFailure.Broadcast(WhyCopy);
		Self->SetReadyToDestroy();
	});
}
