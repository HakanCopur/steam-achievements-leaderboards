// Fill out your copyright notice in the Description page of Project Settings.


#include "SteamSALBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "SALTypes.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY_STATIC(LogSteamSAL, Log, All);

bool USteamSALBlueprintLibrary::IsSteamAvailable(const UObject* WorldContextObject)
{
	// World context sanity
	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL_IsSteamAvailable] WorldContextObject/World is null"));
		return false;
	}

	// Gather Steam interface state
	const bool bHasUser = (SteamUser() != nullptr);
	const bool bHasUserStats = (SteamUserStats() != nullptr);
	const bool bHasUtils = (SteamUtils() != nullptr);

	const uint32 AppID = SteamUtils() ? SteamUtils()->GetAppID() : 0;
	const bool bLoggedOn = SteamUser() ? SteamUser()->BLoggedOn() : false;

	// (Optional) See which Online Subsystem is active; don't gate on it, just log for clarity
	const IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	const FString OSSName = OSS ? OSS->GetSubsystemName().ToString() : TEXT("<null>");

	// Decide availability:
	// - Interfaces must exist
	// - AppID must be non-zero
	// - Logged-on is nice to have; if you want to allow offline testing, you can drop this from the AND chain
	const bool bAvailable = bHasUser && bHasUserStats && bHasUtils && (AppID != 0) && bLoggedOn;

	// Helpful logs
	UE_LOG(LogTemp, Log, TEXT("[SAL_IsSteamAvailable] IsSteamAvailable() = %s"),
	       bAvailable ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log, TEXT("[SAL_IsSteamAvailable] OSS=%s  AppID=%u  SteamUserStats=%s  SteamUser=%s  LoggedOn=%s"),
	       *OSSName,
	       AppID,
	       bHasUserStats ? TEXT("OK") : TEXT("null"),
	       bHasUser ? TEXT("OK") : TEXT("null"),
	       bLoggedOn ? TEXT("true") : TEXT("false"));

	return bAvailable;
}

void USteamSALBlueprintLibrary::GetSteamID64FromController(APlayerController* PlayerController, FString& SteamID)
{
	SteamID.Reset();

	if (!PlayerController)
	{
		return;
	}

	const APlayerState* PS = PlayerController->PlayerState;
	if (!PS)
	{
		return;
	}

	const FUniqueNetIdRepl& Repl = PS->GetUniqueId();
	if (!Repl.IsValid())
	{
		return;
	}

	SteamID = Repl->ToString();
	return;
}

static bool SAL_IsValidHandle(const FSAL_LeaderboardHandle& H)
{
	return H.Value != 0 && SteamUserStats() != nullptr;
}

FString USteamSALBlueprintLibrary::GetLeaderboardName(const FSAL_LeaderboardHandle& LeaderboardHandle)
{
	if (SAL_IsValidHandle(LeaderboardHandle))
	{
		const char* Name = SteamUserStats()->GetLeaderboardName((SteamLeaderboard_t)LeaderboardHandle.Value);
		return Name ? UTF8_TO_TCHAR(Name) : TEXT("");
	}
	return TEXT("Invalid");
}

int32 USteamSALBlueprintLibrary::GetLeaderboardEntryCount(const FSAL_LeaderboardHandle& LeaderboardHandle)
{
	if (SAL_IsValidHandle(LeaderboardHandle))
	{
		return SteamUserStats()->GetLeaderboardEntryCount((SteamLeaderboard_t)LeaderboardHandle.Value);
	}
	return 0;
}

ESALLeaderboardSortMethod USteamSALBlueprintLibrary::GetLeaderboardSortMethod(
	const FSAL_LeaderboardHandle& LeaderboardHandle)
{
	if (SAL_IsValidHandle(LeaderboardHandle))
	{
		switch (SteamUserStats()->GetLeaderboardSortMethod((SteamLeaderboard_t)LeaderboardHandle.Value))
		{
		case k_ELeaderboardSortMethodAscending: return ESALLeaderboardSortMethod::Ascending;
		case k_ELeaderboardSortMethodDescending: return ESALLeaderboardSortMethod::Descending;
		}
	}
	return ESALLeaderboardSortMethod::Descending;
}

ESALLeaderboardDisplayType USteamSALBlueprintLibrary::GetLeaderboardDisplayType(
	const FSAL_LeaderboardHandle& LeaderboardHandle)
{
	if (SAL_IsValidHandle(LeaderboardHandle))
	{
		switch (SteamUserStats()->GetLeaderboardDisplayType((SteamLeaderboard_t)LeaderboardHandle.Value))
		{
		case k_ELeaderboardDisplayTypeNumeric: return ESALLeaderboardDisplayType::Numeric;
		case k_ELeaderboardDisplayTypeTimeSeconds: return ESALLeaderboardDisplayType::TimeSeconds;
		case k_ELeaderboardDisplayTypeTimeMilliSeconds: return ESALLeaderboardDisplayType::TimeMilliSeconds;
		}
	}
	return ESALLeaderboardDisplayType::Numeric;
}

bool USteamSALBlueprintLibrary::SetAchievement(const FString& AchievementAPIName)
{
	if (!SteamUserStats())
		return false;

	return SteamUserStats()->SetAchievement(TCHAR_TO_ANSI(*AchievementAPIName));
}

bool USteamSALBlueprintLibrary::ClearAchievement(const FString& AchievementAPIName)
{
	if (!SteamUserStats())
		return false;

	return SteamUserStats()->ClearAchievement(TCHAR_TO_ANSI(*AchievementAPIName));
}

void USteamSALBlueprintLibrary::GetAchievementStatus(const FString& AchievementAPIName, bool& bUnlocked,
                                                     int32& UnlockUnixTime, bool& bSuccess)
{
	bUnlocked = false;
	UnlockUnixTime = 0;
	bSuccess = false;

	if (!SteamUserStats())
	{
		return; // Steam not available
	}

	uint32 UnlockTime = 0;
	if (SteamUserStats()->GetAchievementAndUnlockTime(TCHAR_TO_ANSI(*AchievementAPIName), &bUnlocked, &UnlockTime))
	{
		UnlockUnixTime = static_cast<int32>(UnlockTime);
		bSuccess = true;
	}
}


FString USteamSALBlueprintLibrary::GetAchievementAPIName(int32 AchievementIndex)
{
	if (!SteamUserStats())
	{
		return FString();
	}

	const char* Name = SteamUserStats()->GetAchievementName(AchievementIndex);
	return Name ? FString(ANSI_TO_TCHAR(Name)) : FString();
}


UTexture2D* USteamSALBlueprintLibrary::GetAchievementIcon(const FString& AchievementAPIName, bool bUnlockedIcon)
{
	if (!SteamUserStats() || !SteamUtils())
	{
		return nullptr;
	}

	int IconHandle = SteamUserStats()->GetAchievementIcon(TCHAR_TO_ANSI(*AchievementAPIName));
	if (IconHandle == 0)
	{
		return nullptr;
	}

	uint32 Width, Height;
	if (!SteamUtils()->GetImageSize(IconHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		return nullptr;
	}

	TArray<uint8> RawData;
	RawData.SetNumUninitialized(Width * Height * 4);

	if (!SteamUtils()->GetImageRGBA(IconHandle, RawData.GetData(), RawData.Num()))
	{
		return nullptr;
	}

	// Create texture
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		return nullptr;
	}

	// Lock & copy data
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

	// Update resource
	Texture->UpdateResource();

	return Texture;
}

void USteamSALBlueprintLibrary::GetAchievementDisplayInfo(const FString& AchievementAPIName, FString& DisplayName,
                                                          FString& Description, bool& bSuccess)
{
	DisplayName = TEXT("");
	Description = TEXT("");
	bSuccess = false;

	if (!SteamUserStats())
	{
		return;
	}

	const char* Name = SteamUserStats()->GetAchievementDisplayAttribute(TCHAR_TO_ANSI(*AchievementAPIName), "name");
	const char* Desc = SteamUserStats()->GetAchievementDisplayAttribute(TCHAR_TO_ANSI(*AchievementAPIName), "desc");

	if (Name) { DisplayName = FString(ANSI_TO_TCHAR(Name)); }
	if (Desc) { Description = FString(ANSI_TO_TCHAR(Desc)); }

	bSuccess = (Name != nullptr || Desc != nullptr);
}

void USteamSALBlueprintLibrary::GetGlobalAchievementPercent(const FString& AchievementAPIName, float& Percent,
                                                            bool& bSuccess)
{
	Percent = 0.0f;
	bSuccess = false;

	if (!SteamUserStats())
	{
		return;
	}

	float Pct = 0.0f;
	if (SteamUserStats()->GetAchievementAchievedPercent(TCHAR_TO_ANSI(*AchievementAPIName), &Pct))
	{
		Percent = Pct;
		bSuccess = true;
	}
}

void USteamSALBlueprintLibrary::GetNumberOfAchievements(int32& Count, bool& bSuccess)
{
	Count = 0;
	bSuccess = false;

	if (!SteamUserStats())
	{
		return;
	}

	Count = SteamUserStats()->GetNumAchievements();
	bSuccess = true;
}

void USteamSALBlueprintLibrary::IndicateAchievementProgress(FName AchievementAPIName,
                                                            int32 CurrentProgress, int32 MaxProgress, bool& bSuccess)
{
	bSuccess = false;

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] IndicateAchievementProgress: SteamUserStats unavailable"));
		return;
	}

	if (AchievementAPIName.IsNone() || MaxProgress <= 0 || CurrentProgress < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] IndicateAchievementProgress: Invalid params (Name=%s, Cur=%d, Max=%d)"),
		       *AchievementAPIName.ToString(), CurrentProgress, MaxProgress);
		return;
	}

	const FString NameStr = AchievementAPIName.ToString();
	const ANSICHAR* AnsiName = TCHAR_TO_ANSI(*NameStr);

	// Clamp progress values
	uint32 Cur = static_cast<uint32>(FMath::Clamp(CurrentProgress, 0, MaxProgress));
	uint32 Max = static_cast<uint32>(MaxProgress);

	// Show progress toast
	bool bIndicated = SteamUserStats()->IndicateAchievementProgress(AnsiName, Cur, Max);

	// Auto-unlock when complete
	if (bIndicated && Cur >= Max)
	{
		bool bUnlocked = false;
		SteamUserStats()->GetAchievement(AnsiName, &bUnlocked);

		if (!bUnlocked)
		{
			bool bSet = SteamUserStats()->SetAchievement(AnsiName);
			bool bStore = SteamUserStats()->StoreStats();
			bIndicated = bIndicated && bSet && bStore;
		}
	}

	bSuccess = bIndicated;
}

void USteamSALBlueprintLibrary::GetStoredStat(const FString& StatAPIName, ESALStatReadType StatType,
                                              int32& IntegerValue, float& FloatValue, bool& bSuccess)
{
	IntegerValue = 0;
	FloatValue = 0.0f;
	bSuccess = false;

	if (!SteamUserStats())
	{
		return; // Steam not available (ensure Steam + RequestCurrentStats succeeded first)
	}

	const char* NameAnsi = TCHAR_TO_ANSI(*StatAPIName);

	switch (StatType)
	{
	case ESALStatReadType::Integer:
		{
			int32 Tmp = 0;
			if (SteamUserStats()->GetStat(NameAnsi, &Tmp))
			{
				IntegerValue = Tmp;
				bSuccess = true;
			}
			break;
		}
	case ESALStatReadType::Float:
	case ESALStatReadType::Average: // average-rate stats are retrieved as float
		{
			float Tmp = 0.0f;
			if (SteamUserStats()->GetStat(NameAnsi, &Tmp))
			{
				FloatValue = Tmp;
				bSuccess = true;
			}
			break;
		}
	default: break;
	}
}

void USteamSALBlueprintLibrary::GetStoredStats(const TArray<FSAL_StatQuery>& StatsToGet,
                                               TArray<FSAL_StoredStat>& StatsOut,
                                               bool& bAllSucceeded)
{
	StatsOut.Empty();
	bAllSucceeded = false;

	if (!SteamUserStats())
	{
		return; // Steam not available. Make sure Request Current Stats succeeded earlier.
	}

	StatsOut.Reserve(StatsToGet.Num());
	bool AllOK = true;

	for (const FSAL_StatQuery& Q : StatsToGet)
	{
		FSAL_StoredStat Out;
		Out.APIStatName = Q.APIStatName;
		Out.FriendlyStatName = Q.FriendlyStatName.IsEmpty() ? Q.APIStatName : Q.FriendlyStatName;

		// Treat Average as Float (if you kept it in your enum)
		Out.StatType = (Q.StatType == ESALStatReadType::Average) ? ESALStatReadType::Float : Q.StatType;

		const char* NameAnsi = TCHAR_TO_ANSI(*Q.APIStatName);

		bool Ok = false;
		if (Out.StatType == ESALStatReadType::Integer)
		{
			int32 V = 0;
			Ok = SteamUserStats()->GetStat(NameAnsi, &V);
			if (Ok) { Out.IntegerValue = V; }
		}
		else // Float (and Average treated as Float)
		{
			float V = 0.0f;
			Ok = SteamUserStats()->GetStat(NameAnsi, &V);
			if (Ok) { Out.FloatValue = V; }
		}

		Out.bSucceeded = Ok;
		AllOK = AllOK && Ok;
		StatsOut.Add(Out);
	}

	bAllSucceeded = AllOK;
}

void USteamSALBlueprintLibrary::SetStoredStat(
	const FString& StatAPIName,
	ESALStatReadType StatType,
	int32 IntegerValue,
	float FloatValue,
	float CountThisSession,
	float SessionLengthSeconds,
	bool& bSuccess)
{
	bSuccess = false;

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: SteamUserStats unavailable"));
		return;
	}

	if (StatAPIName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: Empty StatAPIName"));
		return;
	}

	const ANSICHAR* AnsiName = TCHAR_TO_ANSI(*StatAPIName);

	switch (StatType)
	{
	case ESALStatReadType::Integer:
		{
			const bool ok = SteamUserStats()->SetStat(AnsiName, IntegerValue);
			if (!ok) UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: SetStat(int) failed for '%s' (%d)"), *StatAPIName, IntegerValue);
			bSuccess = ok;
			break;
		}
	case ESALStatReadType::Float:
		{
			const bool ok = SteamUserStats()->SetStat(AnsiName, FloatValue);
			if (!ok) UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: SetStat(float) failed for '%s' (%f)"), *StatAPIName, FloatValue);
			bSuccess = ok;
			break;
		}
	case ESALStatReadType::Average:
		{
			if (SessionLengthSeconds <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: AvgRate requires SessionLengthSeconds > 0 for '%s'"), *StatAPIName);
				return;
			}
			const bool ok = SteamUserStats()->UpdateAvgRateStat(AnsiName, CountThisSession, SessionLengthSeconds);
			if (!ok) UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: UpdateAvgRateStat failed for '%s' (Count=%f, Sec=%f)"),
							*StatAPIName, CountThisSession, SessionLengthSeconds);
			bSuccess = ok;
			break;
		}
	default:
		{
			UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStat: Unsupported StatType for '%s'"), *StatAPIName);
			break;
		}
	}

}

void USteamSALBlueprintLibrary::SetStoredStats(
	const TArray<FSAL_StatWrite>& StatsToSet,
	bool& bAllSucceeded)
{
	bAllSucceeded = false;

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStats: SteamUserStats unavailable"));
		return;
	}

	if (StatsToSet.Num() == 0)
	{
		// Nothing to do; consider it a success
		bAllSucceeded = true;
		return;
	}

	bool bAnyFail = false;
	for (const FSAL_StatWrite& W : StatsToSet)
	{
		bool bOne = false;
		USteamSALBlueprintLibrary::SetStoredStat(
			W.APIStatName,
			W.StatType,
			W.IntegerValue,
			W.FloatValue,
			W.CountThisSession,
			W.SessionLengthSeconds,
			bOne
		);
		if (!bOne) { bAnyFail = true; }
	}

	bAllSucceeded = !bAnyFail;

	if (!bAllSucceeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] SetStoredStats: one or more writes failed"));
	}
	
}

void USteamSALBlueprintLibrary::ClearUserStats(bool bAlsoResetAchievements, bool& bSuccess)
{
    bSuccess = false;


    if (SteamUserStats() == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] ClearUserStats: SteamUserStats unavailable"));
        return;
    }

    const bool ok = SteamUserStats()->ResetAllStats(bAlsoResetAchievements);
    if (!ok)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] ClearUserStats: ResetAllStats(%s) returned false"),
            bAlsoResetAchievements ? TEXT("true") : TEXT("false"));
        return;
    }
	
    bSuccess = true;
	
}

void USteamSALBlueprintLibrary::ShowAchievementsOverlay(bool& bSuccess)
{
	bSuccess = false;

	if (!SteamUtils() || !SteamFriends())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] ShowAchievementsOverlay: Steam interfaces unavailable"));
		return;
	}

	if (!SteamUtils()->IsOverlayEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] ShowAchievementsOverlay: Steam overlay is disabled"));
		return;
	}

	// Opens the overlay to the Achievements page of the current app.
	SteamFriends()->ActivateGameOverlay("Achievements");
	bSuccess = true;

}

void USteamSALBlueprintLibrary::GetGlobalStat(
	const FString& StatAPIName,
	ESALStatReadType StatType,
	FString& ValueAsString,
	bool& bSuccess)
{
	bSuccess = false;
	ValueAsString.Empty();

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStat: SteamUserStats unavailable"));
		return;
	}
	if (StatAPIName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStat: Empty StatAPIName"));
		return;
	}

	const ANSICHAR* AnsiName = TCHAR_TO_ANSI(*StatAPIName);

	if (StatType == ESALStatReadType::Integer)
	{
		int64 Val = 0;
		if (SteamUserStats()->GetGlobalStat(AnsiName, &Val))
		{
			ValueAsString = FString::Printf(TEXT("%lld"), Val);
			bSuccess = true;
		}
	}
	else if (StatType == ESALStatReadType::Float || StatType == ESALStatReadType::Average)
	{
		double Val = 0.0;
		if (SteamUserStats()->GetGlobalStat(AnsiName, &Val))
		{
			ValueAsString = FString::SanitizeFloat(Val);
			bSuccess = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStat: Unsupported StatType for '%s'"), *StatAPIName);
	}
	
}


void USteamSALBlueprintLibrary::GetGlobalStatHistory(
	const FString& StatAPIName,
	ESALStatReadType StatType,
	int32 NumSamplesRequested,
	TArray<FString>& HistoryValues,
	bool& bSuccess)
{
	bSuccess = false;
	HistoryValues.Empty();

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStatHistory: SteamUserStats unavailable"));
		return;
	}

	if (StatAPIName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStatHistory: Empty StatAPIName"));
		return;
	}

	const ANSICHAR* AnsiName = TCHAR_TO_ANSI(*StatAPIName);

	if (StatType == ESALStatReadType::Integer)
	{
		TArray<int64> Buffer;
		Buffer.SetNumUninitialized(NumSamplesRequested);

		int32 NumGot = SteamUserStats()->GetGlobalStatHistory(
			AnsiName,
			Buffer.GetData(),
			NumSamplesRequested
		);

		if (NumGot > 0)
		{
			for (int32 i = 0; i < NumGot; i++)
			{
				HistoryValues.Add(FString::Printf(TEXT("%lld"), Buffer[i]));
			}
			bSuccess = true;
		}
	}
	else if (StatType == ESALStatReadType::Float || StatType == ESALStatReadType::Average)
	{
		TArray<double> Buffer;
		Buffer.SetNumUninitialized(NumSamplesRequested);

		int32 NumGot = SteamUserStats()->GetGlobalStatHistory(
			AnsiName,
			Buffer.GetData(),
			NumSamplesRequested
		);

		if (NumGot > 0)
		{
			for (int32 i = 0; i < NumGot; i++)
			{
				HistoryValues.Add(FString::SanitizeFloat(Buffer[i]));
			}
			bSuccess = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] GetGlobalStatHistory: Unsupported StatType for '%s'"), *StatAPIName);
	}

}

void USteamSALBlueprintLibrary::AddToStoredStat(
    const FString& StatAPIName,
    ESALStatReadType StatType,
    float Delta,
    float& NewValue,
    bool& bSuccess)
{
    bSuccess = false;
    NewValue = 0.0f;
	
    if (SteamUserStats() == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: SteamUserStats unavailable"));
        return;
    }
    if (StatAPIName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: Empty StatAPIName"));
        return;
    }

    const ANSICHAR* AnsiName = TCHAR_TO_ANSI(*StatAPIName);

    if (StatType == ESALStatReadType::Integer)
    {
        int32 Current = 0;
        if (!SteamUserStats()->GetStat(AnsiName, &Current))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: GetStat(int) failed for '%s'"), *StatAPIName);
            return;
        }

        int32 Updated = Current + static_cast<int32>(Delta);
        if (!SteamUserStats()->SetStat(AnsiName, Updated))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: SetStat(int) failed for '%s' (%d)"), *StatAPIName, Updated);
            return;
        }

        NewValue = static_cast<float>(Updated);
        bSuccess = true;
    }
    else if (StatType == ESALStatReadType::Float)
    {
        float Current = 0.0f;
        if (!SteamUserStats()->GetStat(AnsiName, &Current))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: GetStat(float) failed for '%s'"), *StatAPIName);
            return;
        }

        float Updated = Current + Delta;
        if (!SteamUserStats()->SetStat(AnsiName, Updated))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: SetStat(float) failed for '%s' (%f)"), *StatAPIName, Updated);
            return;
        }

        NewValue = Updated;
        bSuccess = true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SAL] AddToStoredStat: Unsupported StatType for '%s' (only Integer/Float allowed)"), *StatAPIName);
    }
	
}

void USteamSALBlueprintLibrary::ListAllAchievementAPINames(
	TArray<FString>& AchievementAPINames, bool& bSuccess)
{
	bSuccess = false;
	AchievementAPINames.Reset();

	if (SteamUserStats() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SAL] ListAllAchievementAPINames: SteamUserStats unavailable"));
		return;
	}

	const int32 Count = SteamUserStats()->GetNumAchievements();
	if (Count <= 0)
	{
		// No achievements defined or schema not loaded yet
		bSuccess = (Count == 0); // true if empty list is expected
		return;
	}

	AchievementAPINames.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const char* NameAnsi = SteamUserStats()->GetAchievementName(Index);
		if (NameAnsi && NameAnsi[0] != '\0')
		{
			AchievementAPINames.Add(FString(UTF8_TO_TCHAR(NameAnsi)));
		}
	}

	bSuccess = true;

}






