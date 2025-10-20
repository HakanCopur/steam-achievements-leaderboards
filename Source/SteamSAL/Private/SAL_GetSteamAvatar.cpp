// Copyright (c) 2025 UnForge. All rights reserved.

#include "SAL_GetSteamAvatar.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "PixelFormat.h"
#include "Serialization/BulkData.h"

TMap<USAL_GetSteamAvatar::FAvatarKey, TWeakObjectPtr<UTexture2D>> USAL_GetSteamAvatar::AvatarCache;

USAL_GetSteamAvatar* USAL_GetSteamAvatar::GetSteamAvatar(UObject* WorldContextObject, const FString& SteamID64, ESALAvatarSize Size)
{
    USAL_GetSteamAvatar* Node = NewObject<USAL_GetSteamAvatar>();
    Node->RegisterWithGameInstance(WorldContextObject);

    Node->WorldContextObject = WorldContextObject;
    Node->InSteamID64 = SteamID64;
    Node->InSize = Size;

    return Node;
}

void USAL_GetSteamAvatar::Activate()
{
    if (InSteamID64.IsEmpty())
    {
        BroadcastFailure(TEXT("GetSteamAvatar: SteamID64 is empty."));
        return;
    }

    if (SteamFriends() == nullptr || SteamUtils() == nullptr)
    {
        BroadcastFailure(TEXT("GetSteamAvatar: Steam not available or not initialized."));
        return;
    }

    // Cache hit?
    const FAvatarKey Key{ InSteamID64, InSize };
    if (const TWeakObjectPtr<UTexture2D>* Found = AvatarCache.Find(Key))
    {
        if (Found->IsValid())
        {
            BroadcastSuccess(Found->Get());
            return;
        }
    }

    const CSteamID TargetId = ToCSteamID(InSteamID64);
    if (!TargetId.IsValid())
    {
        BroadcastFailure(FString::Printf(TEXT("GetSteamAvatar: Invalid SteamID64 '%s'."), *InSteamID64));
        return;
    }

    // Ask Steam for the avatar image handle (may be not ready yet)
    const int ImageHandle = GetAvatarImageHandle(TargetId);
    if (ImageHandle > 0)
    {
        UTexture2D* Tex = nullptr;
        if (TryGetTextureFromSteamImage(ImageHandle, Tex) && Tex)
        {
            AvatarCache.Add(Key, Tex);
            BroadcastSuccess(Tex);
            return;
        }
        // If retrieval failed even with a valid handle, fall back to polling a bit
    }
    else
    {
        // Not ready (Steam may return 0 or k_unImageIdInvalid); request persona info to trigger download
        SteamFriends()->RequestUserInformation(TargetId, true);
    }

    // Poll briefly until the image becomes available
    StartPoll();
}

CSteamID USAL_GetSteamAvatar::ToCSteamID(const FString& SteamIDStr) const
{
    // Expecting a 64-bit decimal string (same as you store in FSAL_LeaderboardEntryRow.SteamID)
    uint64 Id64 = 0;
    LexFromString(Id64, *SteamIDStr);
    return CSteamID(Id64);
}

int USAL_GetSteamAvatar::GetAvatarImageHandle(CSteamID SteamId) const
{
    switch (InSize)
    {
        case ESALAvatarSize::Small:  return SteamFriends()->GetSmallFriendAvatar(SteamId);
        case ESALAvatarSize::Medium: return SteamFriends()->GetMediumFriendAvatar(SteamId);
        case ESALAvatarSize::Large:  return SteamFriends()->GetLargeFriendAvatar(SteamId);
        default: return SteamFriends()->GetMediumFriendAvatar(SteamId);
    }
}

bool USAL_GetSteamAvatar::TryGetTextureFromSteamImage(int ImageHandle, UTexture2D*& OutTexture)
{
    OutTexture = nullptr;

    if (ImageHandle <= 0 || SteamUtils() == nullptr)
    {
        return false;
    }

    uint32 Width = 0, Height = 0;
    if (!SteamUtils()->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
    {
        return false;
    }

    // Steam returns RGBA8; Unreal's common path uses BGRA8. We'll convert.
    TArray<uint8> RGBA;
    RGBA.SetNumUninitialized(Width * Height * 4);

    if (!SteamUtils()->GetImageRGBA(ImageHandle, RGBA.GetData(), RGBA.Num()))
    {
        return false;
    }

    // Convert RGBA -> BGRA in-place to a new buffer
    TArray<uint8> BGRA;
    BGRA.SetNumUninitialized(RGBA.Num());
    for (uint32 i = 0; i < Width * Height; ++i)
    {
        const uint8 R = RGBA[i * 4 + 0];
        const uint8 G = RGBA[i * 4 + 1];
        const uint8 B = RGBA[i * 4 + 2];
        const uint8 A = RGBA[i * 4 + 3];

        BGRA[i * 4 + 0] = B;
        BGRA[i * 4 + 1] = G;
        BGRA[i * 4 + 2] = R;
        BGRA[i * 4 + 3] = A;
    }

    // Create the texture (PF_B8G8R8A8 to match BGRA)
    UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!IsValid(Tex))
    {
        return false;
    }

    Tex->SRGB = true;

    FTexturePlatformData* PlatformData = Tex->GetPlatformData();
    if (PlatformData == nullptr || PlatformData->Mips.Num() == 0)
    {
        return false;
    }

    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Data, BGRA.GetData(), BGRA.Num());
    Mip.BulkData.Unlock();

    Tex->UpdateResource();

    OutTexture = Tex;
    return true;
}

void USAL_GetSteamAvatar::StartPoll()
{
    if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr)
    {
        PollAttempts = 0;
        World->GetTimerManager().SetTimer(PollHandle, this, &USAL_GetSteamAvatar::PollOnce, PollIntervalSec, true);
    }
    else
    {
        BroadcastFailure(TEXT("GetSteamAvatar: Unable to acquire UWorld for polling."));
    }
}

void USAL_GetSteamAvatar::PollOnce()
{
    if (++PollAttempts > MaxPollAttempts)
    {
        if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
        {
            World->GetTimerManager().ClearTimer(PollHandle);
        }
        BroadcastFailure(TEXT("GetSteamAvatar: Timed out waiting for image data."));
        return;
    }

    const CSteamID TargetId = ToCSteamID(InSteamID64);
    const int ImageHandle = GetAvatarImageHandle(TargetId);
    if (ImageHandle <= 0)
    {
        return; // keep polling
    }

    UTexture2D* Tex = nullptr;
    if (TryGetTextureFromSteamImage(ImageHandle, Tex) && Tex)
    {
        if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
        {
            World->GetTimerManager().ClearTimer(PollHandle);
        }

        const FAvatarKey Key{ InSteamID64, InSize };
        AvatarCache.Add(Key, Tex);

        BroadcastSuccess(Tex);
    }
}

void USAL_GetSteamAvatar::BroadcastFailure(const FString& Why)
{
    const FString WhyCopy = Why;
    TWeakObjectPtr<USAL_GetSteamAvatar> Self(this);

    SAL_RunOnGameThread([Self, WhyCopy]()
    {
        if (!Self.IsValid()) return;
        Self->OnFailure.Broadcast(WhyCopy);
        Self->SetReadyToDestroy();
    });
}

void USAL_GetSteamAvatar::BroadcastSuccess(UTexture2D* Texture)
{
    TWeakObjectPtr<USAL_GetSteamAvatar> Self(this);
    TWeakObjectPtr<UTexture2D> Tex(Texture);

    SAL_RunOnGameThread([Self, Tex]()
    {
        if (!Self.IsValid()) return;
        Self->OnSuccess.Broadcast(Tex.Get());
        Self->SetReadyToDestroy();
    });
}
