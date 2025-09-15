// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_StoreStatsAndAchievements.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSAL_StoreStatsAndAchievementsSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_StoreStatsAndAchievementsFailure, FString, ErrorMessage);

UCLASS()
class STEAMSAL_API USAL_StoreStatsAndAchievements : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Bootstrap",
		meta=(WorldContext="WorldContextObject",
			  BlueprintInternalUseOnly="true",
			  DisplayName="Store User Stats & Achievements",
			  ToolTip="Commits pending stat/achievement changes to Steam and triggers the overlay when applicable.",
			  Keywords="steam stats achievements store commit upload"))
	static USAL_StoreStatsAndAchievements* StoreUserStatsAndAchievements(const UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable) FSAL_StoreStatsAndAchievementsSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable) FSAL_StoreStatsAndAchievementsFailure OnFailure;

	virtual void Activate() override;
	
private:
	UPROPERTY() UObject* WorldContextObject = nullptr;
	bool bCompleted = false; // ensure single success

	// Steam manual callbacks (StoreStats has no SteamAPICall_t; we listen for these)
	STEAM_CALLBACK_MANUAL(USAL_StoreStatsAndAchievements, OnUserStatsStored,        UserStatsStored_t,        UserStatsStoredCb);
	STEAM_CALLBACK_MANUAL(USAL_StoreStatsAndAchievements, OnUserAchievementStored,  UserAchievementStored_t,  UserAchievementStoredCb);

	void UnregisterAll()
	{
		UserStatsStoredCb.Unregister();
		UserAchievementStoredCb.Unregister();
	}
};
