// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SALTypes.h"

#include "SteamSALBlueprintLibrary.generated.h"

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
			ToolTip="Gets the user's SteamID64 as a 17-digit string. Outputs empty string if unavailable.",
			Keywords="steam steamid id64 UniqueNetId player controller"
		))
	static void GetSteamID64FromController(
		APlayerController* PlayerController,
		UPARAM(meta=(DisplayName="SteamID (String)"))
		FString& SteamID);

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
	static void SetAchievement(const FString& AchievementAPIName, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Clear Achievement",
			ToolTip=
			"Resets the unlock state of an achievement in memory. Call 'Store User Stats & Achievements' afterwards to persist."
			,
			Keywords="steam achievement reset clear remove revoke"
		))
	static void ClearAchievement(const FString& AchievementAPIName, bool& bSuccess);

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
	static void GetAchievementAPIName(int32 AchievementIndex, FString& APIName, bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="Get Achievement Icon",
			ReturnDisplayName="Icon Texture",
			ToolTip="Retrieves the Steam-provided icon texture for the given achievement."
		))
	static UTexture2D* GetAchievementIcon(const FString& AchievementAPIName);

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
		meta=(
			DisplayName="Indicate Achievement Progress",
			ToolTip=
			"Shows a Steam overlay progress toast (Current/Max). Auto-unlocks when Current ≥ Max. Requires MaxProgress > 0."
			,
			Keywords="steam achievement progress unlock toast notify overlay"
		))
	static void IndicateAchievementProgress(
		UPARAM(meta=(DisplayName="Achievement API Name"))
		FName AchievementAPIName,
		int32 CurrentProgress,
		int32 MaxProgress,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);


	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Get Local (Cached) Stat",
			ToolTip=
			"Reads a stat from the local cache (after a successful 'Request Current Stats And Achievements'). Select the Stat Type; value is returned in the matching output."
			,
			Keywords="steam stats get local cached stored integer float average"
		))
	static void GetStoredStat(
		const FString& StatAPIName, ESALStatReadType StatType,
		UPARAM(meta=(DisplayName="Integer Value"))
		int32& IntegerValue,
		UPARAM(meta=(DisplayName="Float Value"))
		float& FloatValue,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Get Local (Cached) Stats (Batch)",
			ToolTip=
			"Reads multiple stats from the local cache (after Request Current Stats And Achievements). Provide API names and types; returns an array you can Break."
			,
			Keywords="steam stats get local cached stored batch multiple array"
		))
	static void GetStoredStats(
		const TArray<FSAL_StatQuery>& StatsToGet,
		TArray<FSAL_StoredStat>& StatsOut,
		UPARAM(meta=(DisplayName="All Succeeded"))
		bool& bAllSucceeded);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Set Local (Cached) Stat",
			ToolTip=
			"Writes a stat into the local Steam cache. Pick Stat Type; fill matching pins (Integer/Float or AvgRate: Count & Seconds). Call 'Store User Stats & Achievements (Async)' to persist."
			,
			Keywords="steam stats set write update modify local cached store average rate avgrate"
		))
	static void SetStoredStat(
		const FString& StatAPIName, ESALStatReadType StatType,
		int32 IntegerValue, float FloatValue,
		float CountThisSession, float SessionLengthSeconds,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Set Local (Cached) Stats (Batch)",
			ToolTip=
			"Writes multiple stats into the local cache. Each entry selects Integer/Float/Average and its value pins. Call 'Store User Stats & Achievements (Async)' to persist."
			,
			Keywords="steam stats set write batch multiple array local cached store average rate avgrate"
		))
	static void SetStoredStats(
		const TArray<FSAL_StatWrite>& StatsToSet,
		UPARAM(meta=(DisplayName="All Succeeded"))
		bool& bAllSucceeded);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Clear User Stats And Achievements",
			ToolTip=
			"Resets ALL user stats to their default values. Optionally also relocks ALL achievements. Dev/testing only. After this, call 'Store User Stats & Achievements (Async)' to persist, then 'Request Current Stats And Achievements' to refresh the cache."
			,
			Keywords="steam stats reset clear wipe defaults achievements dev testing debug danger",
			AdvancedDisplay="bAlsoResetAchievements",
			DefaultValue="false"
		))
	static void ClearUserStats(
		UPARAM(meta=(DisplayName="Also Reset Achievements"))
		bool bAlsoResetAchievements,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);


	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Get Global Stat (Aggregated)",
			ToolTip=
			"Reads a single aggregated (global) stat from Steam's cache. Requires 'Request Global Stats' earlier this session. Returns the value as a string (ints have no decimals; floats are sanitized)."
			,
			Keywords="steam stats global aggregated community total average avgrate string"
		))
	static void GetGlobalStat(
		const FString& StatAPIName, ESALStatReadType StatType,
		UPARAM(meta=(DisplayName="Value (String)"))
		FString& ValueAsString,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Get Global Stat History",
			ToolTip=
			"Reads historical values of a global aggregated stat (after 'Request Global Stats'). Returns stringified values (index 0 = most recent)."
			,
			Keywords="steam stats global aggregated history trend daily array"
		))
	static void GetGlobalStatHistory(
		const FString& StatAPIName, ESALStatReadType StatType,
		int32 NumSamplesRequested,
		UPARAM(meta=(DisplayName="History (String Array)"))
		TArray<FString>& HistoryValues,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);


	UFUNCTION(BlueprintCallable, Category="SteamSAL|Overlay",
		meta=(
			DisplayName="Show Achievements Overlay",
			ToolTip=
			"Opens the Steam overlay to this game's Achievements page. Requires the Steam overlay to be enabled and running under Steam."
			,
			Keywords="steam overlay achievements open show ui page"
		))
	static void ShowAchievementsOverlay(
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="SteamSAL|Stats",
		meta=(
			DisplayName="Add To Local (Cached) Stat",
			ToolTip=
			"Reads a numeric stat from the local cache, adds Delta, and writes it back. Works for Integer and Float types. NewValue is a float; for Integer stats it will be the integer result cast to float. Call 'Store User Stats & Achievements (Async)' to persist."
			,
			Keywords="steam stats add increment increase int integer float delta local cached store"
		))
	static void AddToStoredStat(
		const FString& StatAPIName, ESALStatReadType StatType,
		float Delta,
		UPARAM(meta=(DisplayName="New Value"))
		float& NewValue,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);


	UFUNCTION(BlueprintPure, Category="SteamSAL|Achievements",
		meta=(
			DisplayName="List All Achievement API Names",
			ToolTip=
			"Returns all achievement API identifiers defined for this app (requires a successful 'Request Current Stats And Achievements' earlier this session)."
			,
			Keywords="steam achievements list all api names ids schema"
		))
	static void ListAllAchievementAPINames(
		UPARAM(meta=(DisplayName="Achievement API Names"))
		TArray<FString>& AchievementAPINames,
		UPARAM(meta=(DisplayName="Success"))
		bool& bSuccess);
};
