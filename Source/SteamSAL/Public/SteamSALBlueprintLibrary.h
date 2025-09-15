// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SALTypes.h"

#include "SteamSALBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class STEAMSAL_API USteamSALBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="SteamSAL",
		meta=(WorldContext="WorldContextObject",
			ToolTip="Returns true if Steam is initialized and available on this runtime.",
			Keywords="Steam Available Initialized Online Subsystem"))
	static bool IsSteamAvailable(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Identity",
		meta=(
			DisplayName="Get SteamID",
			ToolTip="Gets the SteamID64 string (17-digit) for this player. Returns false if invalid.",
			Keywords="Steam SteamID ID64 UniqueNetId Player Controller"
		))
	static void GetSteamID64FromController(APlayerController* PlayerController, FString& SteamID);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Leaderboard",
		meta=(
			DisplayName="Get Leaderboard Name",
			ToolTip="Returns the display name of the specified leaderboard.",
			Keywords="Steam Leaderboard Name Title"
		))
	static FString GetLeaderboardName(const FSAL_LeaderboardHandle& LeaderboardHandle);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Leaderboard",
		meta=(
			DisplayName="Get Leaderboard Entry Count",
			ToolTip="Returns the total number of entries currently on this leaderboard.",
			Keywords="Steam Leaderboard Entries Count Size Length"
		))
	static int32 GetLeaderboardEntryCount(const FSAL_LeaderboardHandle& LeaderboardHandle);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Leaderboard",
		meta=(
			DisplayName="Get Leaderboard Sort Method",
			ToolTip="Returns the sorting method of this leaderboard (Ascending or Descending).",
			Keywords="Steam Leaderboard Sort Ascending Descending Order"
		))
	static ESALLeaderboardSortMethod GetLeaderboardSortMethod(const FSAL_LeaderboardHandle& LeaderboardHandle);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Leaderboard",
		meta=(
			DisplayName="Get Leaderboard Display Type",
			ToolTip="Returns the display type of this leaderboard (Numeric, TimeSeconds, or Milliseconds).",
			Keywords="Steam Leaderboard Display Numeric Time Seconds Milliseconds"
		))
	static ESALLeaderboardDisplayType GetLeaderboardDisplayType(const FSAL_LeaderboardHandle& LeaderboardHandle);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Set Achievement (Unlock)",
			ToolTip=
			"Unlocks an achievement in memory. Call 'Store User Stats & Achievements' afterwards to persist and trigger overlay."
			,
			Keywords="steam achievement unlock set add complete"
		))
	static bool SetAchievement(const FString& AchievementAPIName);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Clear Achievement",
			ToolTip=
			"Resets the unlock state of an achievement in memory. Call 'Store User Stats & Achievements' afterwards to persist."
			,
			Keywords="steam achievement reset clear remove revoke"
		))
	static bool ClearAchievement(const FString& AchievementAPIName);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Get Achievement Status",
			ToolTip="Returns whether the specified achievement is unlocked, and the unlock time if available.",
			Keywords="steam achievement get check status unlocked time"
		))
	static void GetAchievementStatus(const FString& AchievementAPIName, bool& bUnlocked, int32& UnlockUnixTime,
	                                 bool& bSuccess);


	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Get Achievement API Name",
			ToolTip="Returns the API (backend) identifier string for the specified achievement index.",
			Keywords="steam achievement api id backend name"
		))
	static FString GetAchievementAPIName(int32 AchievementIndex);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Get Achievement Icon",
			ToolTip="Retrieves the Steam-provided icon (as a texture) for the given achievement.",
			Keywords="steam achievement icon texture image"
		))
	static UTexture2D* GetAchievementIcon(const FString& AchievementAPIName, bool bUnlockedIcon);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Get Achievement Display Info",
			ToolTip="Gets the display name and description for the specified achievement from Steam.",
			Keywords="steam achievement display name description text"
		))
	static void GetAchievementDisplayInfo(const FString& AchievementAPIName, FString& DisplayName, FString& Description,
	                                      bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(DisplayName="Get Global Achievement Percent",
			ToolTip="Reads the GLOBAL unlock percentage for the given achievement (0–100).",
			Keywords="steam achievement percent global rate statistics"))
	static void GetGlobalAchievementPercent(const FString& AchievementAPIName, float& Percent, bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(DisplayName="Get Number of Achievements",
			ToolTip="Returns how many achievements exist for this app from the local schema.",
			Keywords="steam achievements count total num"))
	static void GetNumberOfAchievements(int32& Count, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Achievements",
		meta=(WorldContext="WorldContextObject",
			DisplayName="Indicate Achievement Progress",
			ToolTip="Show Steam overlay progress for the given AchievementName and auto-unlock when complete.",
			Keywords="Steam Achievement Progress Unlock Toast Notify"))
	static void IndicateAchievementProgress(
		FName AchievementAPIName,
		int32 CurrentProgress,
		int32 MaxProgress,
		bool& bSuccess
	);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(DisplayName="Get Stored Stat",
			ToolTip=
			"Reads a cached stat loaded via 'Request Current Stats'. Select the stat type; value is returned in the corresponding output."
			,
			Keywords="steam stats get stored cached integer float average"))
	static void GetStoredStat(const FString& StatAPIName, ESALStatReadType StatType,
	                          int32& IntegerValue, float& FloatValue, bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Get Stored Stats (Batch)",
			ToolTip=
			"Reads multiple cached stats (after Request Current Stats). Provide API names and read types; returns an array of results you can Break in Blueprint."
			,
			Keywords="steam stats get stored cached batch multiple array"
		))
	static void GetStoredStats(const TArray<FSAL_StatQuery>& StatsToGet, TArray<FSAL_StoredStat>& StatsOut,
	                           bool& bAllSucceeded);
};
