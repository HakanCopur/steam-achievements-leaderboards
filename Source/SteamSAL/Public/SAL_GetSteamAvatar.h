// Copyright (c) 2025 UnForge. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SALTypes.h"
#include "SAL_Internal.h" 
#include "Engine/Texture2D.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

#include "SAL_GetSteamAvatar.generated.h"

UENUM(BlueprintType)
enum class ESALAvatarSize : uint8
{
    Small  UMETA(DisplayName = "Small (32x32)"),
    Medium UMETA(DisplayName = "Medium (64x64)"),
    Large  UMETA(DisplayName = "Large (184x184)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnGetAvatarSuccess, UTexture2D*, Avatar);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAL_OnGetAvatarFailure, FString, ErrorMessage);

/**
 * Gets a user's Steam avatar as a UTexture2D.
 * - Input SteamID is a 64-bit string (same format you use in FSAL_LeaderboardEntryRow.SteamID).
 * - Automatically requests persona info if the image isn't downloaded yet, then polls briefly.
 * - Returns on the GameThread; safe for immediate UI/material usage.
 */
UCLASS()
class STEAMSAL_API USAL_GetSteamAvatar : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SteamSAL|Avatar",
        meta=(WorldContext="WorldContextObject",
              BlueprintInternalUseOnly="true",
              ToolTip="Fetch the Steam avatar texture for a user (Small/Medium/Large).",
              DisplayName="Get Steam Avatar (Async)",
              Keywords="steam avatar profile picture image texture friend persona"))
    static USAL_GetSteamAvatar* GetSteamAvatar(
        UObject* WorldContextObject,
        UPARAM(meta=(ToolTip="Target user's SteamID64 string (e.g., from leaderboard row)."))
        const FString& SteamID64,
        UPARAM(meta=(ToolTip="Requested avatar size (Small/Medium/Large)."))
        ESALAvatarSize Size = ESALAvatarSize::Medium
    );

    UPROPERTY(BlueprintAssignable, Category="SteamSAL|Avatar")
    FSAL_OnGetAvatarSuccess OnSuccess;

    UPROPERTY(BlueprintAssignable, Category="SteamSAL|Avatar")
    FSAL_OnGetAvatarFailure OnFailure;

    virtual void Activate() override;

private:
    // Inputs
    UPROPERTY()
    UObject* WorldContextObject = nullptr;

    FString InSteamID64;
    ESALAvatarSize InSize = ESALAvatarSize::Medium;

    // Simple in-memory cache (SteamID64 + Size)
    struct FAvatarKey
    {
        FString SteamID;
        ESALAvatarSize Size;
        friend uint32 GetTypeHash(const FAvatarKey& K)
        {
            return HashCombine(GetTypeHash(K.SteamID), ::GetTypeHash(static_cast<uint8>(K.Size)));
        }
        bool operator==(const FAvatarKey& Other) const
        {
            return Size == Other.Size && SteamID == Other.SteamID;
        }
    };

    static TMap<FAvatarKey, TWeakObjectPtr<UTexture2D>> AvatarCache;

    // Polling for when image data isn't ready yet
    FTimerHandle PollHandle;
    int32 PollAttempts = 0;
    static constexpr int32 MaxPollAttempts = 30;   // ~3 seconds at 0.1s
    static constexpr float PollIntervalSec = 0.10f;

    // Internal
    CSteamID ToCSteamID(const FString& SteamIDStr) const;
    int GetAvatarImageHandle(CSteamID SteamId) const;
    bool TryGetTextureFromSteamImage(int ImageHandle, UTexture2D*& OutTexture);
    void StartPoll();
    void PollOnce();
    void BroadcastFailure(const FString& Why);
    void BroadcastSuccess(UTexture2D* Texture);
};
