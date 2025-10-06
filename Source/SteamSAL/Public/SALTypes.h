// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SALTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamSAL, Log, All);

UENUM(BlueprintType)
enum class ELeaderboardRequestType : uint8
{
	Global            UMETA(DisplayName="Global", ToolTip="Downloads a global range by absolute ranks [RangeStart..RangeEnd] (1-based)."),
	GlobalAroundUser  UMETA(DisplayName="Global Around User", ToolTip="Downloads entries around the local user; use negative RangeStart and positive RangeEnd (e.g., -5..+5)."),
	Friends           UMETA(DisplayName="Friends", ToolTip="Downloads only Steam friends who have scores; RangeStart/End are ignored.")
};


USTRUCT(BlueprintType)
struct FLeaderboardOpResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SteamSAL")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "SteamSAL")
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "SteamSAL")
	FString Message;
};

USTRUCT(BlueprintType)
struct FLeaderboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Player's SteamID64 (e.g., 7656119...)."))
	FString SteamId;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Player's Steam display name at the time of query."))
	FString PersonaName;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Global rank (1 = top)."))
	int32 Rank = -1;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Score value as stored on the leaderboard."))
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(AdvancedDisplay, ToolTip=
			"Optional array of extra integers supplied during upload. Leave unused unless your game writes them."))
	TArray<int32> Details;
};

UENUM(BlueprintType)
enum class ESALLeaderboardSortMethod : uint8
{
	Ascending UMETA(DisplayName="Ascending", ToolTip="Lower score is better (use for time)."),
	Descending UMETA(DisplayName="Descending", ToolTip="Higher score is better (use for points).")
};

UENUM(BlueprintType)
enum class ESALLeaderboardDisplayType : uint8
{
	Numeric UMETA(DisplayName="Numeric", ToolTip="Show score as a plain integer."),
	TimeSeconds UMETA(DisplayName="Time (Seconds)", ToolTip="Show score formatted as time in seconds."),
	TimeMilliSeconds UMETA(DisplayName="Time (Milliseconds)", ToolTip="Show score formatted as time in milliseconds.")
};

USTRUCT(BlueprintType)
struct FSAL_LeaderboardHandle
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int64 Value = 0;
};

UENUM(BlueprintType)
enum class ESALLeaderboardUploadMethod : uint8
{
	KeepBestScore UMETA(DisplayName = "Keep Best Score (Only Update Score If Better)", ToolTip="Only replaces the stored score if the new score is better according to the leaderboard's sort method."),
	ForceUpdate UMETA(DisplayName = "Force Update Score (Always Overwrite)", ToolTip= "Always updates the stored score, even if the new score is worse.")
};

USTRUCT(BlueprintType)
struct FSAL_LeaderboardEntryRow
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
			  meta=(ToolTip="Player's SteamID64 (e.g., 7656119...)."))
	FString SteamID = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
			  meta=(ToolTip="Global rank (1 = top)."))
	int32 GlobalRank = 0;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
			  meta=(ToolTip="Score value as stored on the leaderboard."))
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
			  meta=(AdvancedDisplay, ToolTip="Optional metadata saved with the score. Visible only if you requested DetailsMax > 0 when downloading. Up to 64 int32 values."))
	TArray<int32> Details;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		  meta=(ToolTip="Player's current Steam display name. May be empty if not cached yet."))
	FString PlayerName;
};

UENUM(BlueprintType)
enum class ESALStatReadType : uint8
{
	Integer UMETA(DisplayName="Integer", ToolTip="Read the stat as an integer value (int32)."),

	Float UMETA(DisplayName="Float", ToolTip="Read the stat as a floating point value (float)."),

	Average UMETA(DisplayName="Average", ToolTip="Read the stat as a float representing an average rate. Use UpdateAvgRateStat to set these.")
};

USTRUCT(BlueprintType)
struct FSAL_StatQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="SteamSAL",
		meta=(ToolTip="Stat API name exactly as defined in Steam (e.g., 'TOTAL_JUMPS')."))
	FString APIStatName = TEXT("");

	UPROPERTY(BlueprintReadWrite, Category="SteamSAL",
		meta=(ToolTip="How to read this stat (Integer or Float). Average stats are read as Float."))
	ESALStatReadType StatType = ESALStatReadType::Integer;

	UPROPERTY(BlueprintReadWrite, Category="SteamSAL",
		meta=(ToolTip="Optional friendly label for UI; if empty, output will use APIStatName."))
	FString FriendlyStatName = TEXT("");
};

USTRUCT(BlueprintType)
struct FSAL_StoredStat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Friendly name for UI (falls back to APIStatName if not provided)."))
	FString FriendlyStatName = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="API name exactly as defined in Steam."))
	FString APIStatName = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Read type used for this value (Integer or Float)."))
	ESALStatReadType StatType = ESALStatReadType::Integer;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Integer value (valid if StatType == Integer)."))
	int32 IntegerValue = 0;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="Float value (valid if StatType == Float)."))
	float FloatValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="SteamSAL",
		meta=(ToolTip="True if this individual read succeeded from cached Steam data."))
	bool bSucceeded = false;
};

USTRUCT(BlueprintType)
struct FSAL_StatWrite
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(ToolTip="Stat API name exactly as defined in Steam (e.g., 'TOTAL_JUMPS')."))
	FString APIStatName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(ToolTip="Write type (Integer, Float, or Average). Average uses CountThisSession & SessionLengthSeconds."))
	ESALStatReadType StatType = ESALStatReadType::Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(EditConditionHides, EditCondition="StatType == ESALStatReadType::Integer",
			  ToolTip="New integer value to set when StatType is Integer."))
	int32 IntegerValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(EditConditionHides, EditCondition="StatType == ESALStatReadType::Float",
			  ToolTip="New float value to set when StatType is Float."))
	float FloatValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(EditConditionHides, EditCondition="StatType == ESALStatReadType::Average",
			  ToolTip="Average stats: amount counted this session (e.g., events)."))
	float CountThisSession = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SteamSAL",
		meta=(EditConditionHides, EditCondition="StatType == ESALStatReadType::Average",
			  ToolTip="Average stats: duration for this measurement in seconds (must be > 0)."))
	float SessionLengthSeconds = 0.0f;
};