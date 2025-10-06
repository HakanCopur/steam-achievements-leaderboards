// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_RequestCurrentStats.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSAL_RequestCurrentStatsSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_RequestCurrentStatsFailure, FString, ErrorMessage);


UCLASS()
class STEAMSAL_API USAL_RequestCurrentStats : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="SteamSAL|Bootstrap",
		meta=(WorldContext="WorldContextObject",
			  BlueprintInternalUseOnly="true",
			  DisplayName="Request Current Stats And Achievements",
			  ToolTip="Requests the local user's Steam stats and achievements. Must complete before any Get/Set/Store calls.",
			  Keywords="steam stats achievements request bootstrap ready"))
	static USAL_RequestCurrentStats* RequestCurrentStats(UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable) FSAL_RequestCurrentStatsSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable) FSAL_RequestCurrentStatsFailure OnFailure;

	virtual void Activate() override;
	
private:
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	CCallResult<USAL_RequestCurrentStats, struct UserStatsReceived_t> CallResult;

	void OnUserStatsReceived(struct UserStatsReceived_t* Callback, bool bIOFailure);
	
};
