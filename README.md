<h1 align="center"><b>SteamSAL — Steam Achievements & Leaderboards for UE5</b></h1>

<p align="center">
  <img src="Resources/Icon128.png" alt="SteamSAL Icon" width="96" height="96" />
</p>

<p align="center">
  Blueprint-first, async-driven, tiny API surface.<br/>
  Supports Unreal Engine <b>5.0 → 5.4</b> on <b>Win64</b>.
</p>

## **Table of Contents**
- [**Table of Contents**](#table-of-contents)
- [**About**](#about)
- [**Features**](#features)
- [**Compatibility**](#compatibility)
- [**Install**](#install)
- [**Enable Steam Online Subsystem (Required)**](#enable-steam-online-subsystem-required)
- [**Quick Start (Leaderboards)**](#quick-start-leaderboards)
- [**Blueprint Nodes Overview**](#blueprint-nodes-overview)
- [**Troubleshooting**](#troubleshooting)
- [**License**](#license)

---

## **About**
**SteamSAL** is a lightweight Unreal Engine 5 plugin that exposes **Steam Leaderboards (and Achievements)** through simple **async Blueprint** nodes—ideal for solo devs and small teams who want the essentials of Steamworks without writing C++.

---

## **Features**
- 🔎 **Find / Create** leaderboards  
- ⬆️ **Upload** scores (Keep Best / Force Update) with optional `Details[]`  
- ⬇️ **Download** entries (Global, Friends, Around User, For Users)  
- 🧩 **Break** rows to SteamID, Rank, Score, Details[], Player Name  
- 🧰 Helpers: `IsSteamAvailable`, `GetSteamID`, `Get Leaderboard Name / Sort / Display / Entry Count`

---

## **Compatibility**
- **Engine:** UE **5.0 → 5.4**  
- **Platform:** **Win64** (others not officially tested)  
- **Plugin Type:** **Runtime** (no content)

---

## **Install**
1. **Copy the plugin**
   - Put the `SteamSAL` folder into:
     - `YourProject/Plugins/SteamSAL` *(recommended)*, **or**
     - `Engine/Plugins/Marketplace/SteamSAL`
2. **Enable the plugin**
   - In **Editor → Edit → Plugins**, search **SteamSAL** and enable it. Restart if prompted.
3. **Enable Steam Online Subsystem**  
   - Required. See next section.

---

## **Enable Steam Online Subsystem (Required)**
Follow Epic’s official guide (**select your exact UE version** in the doc’s version switcher):  
**▶ Online Subsystem Steam:** https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-steam-interface-in-unreal-engine

**Minimal `Config/DefaultEngine.ini` (UE 5.0–5.4):**
```ini
; Enable Steam as the default online subsystem
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
; Use your own AppID for testing (480 = Spacewar sample)
SteamDevAppId=480

; Use Steam net driver
[/Script/Engine.GameEngine]
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/Engine.NetDriver")

[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"
```

> **Local testing tips**
> - Put `steam_appid.txt` (containing your AppID, e.g. `480`) next to the executable you launch (editor or packaged).  
> - Ensure the **Steam client is running** before starting the game.  
> - **Do not** ship `steam_appid.txt` in releases.

---

## **Quick Start (Leaderboards)**
1. Call **`Is Steam Available`** → proceed only if `true`.  
2. **`Find Steam Leaderboard`** (or **`Create Steam Leaderboard`**) → keep the returned **Leaderboard Handle**.  
3. **`Upload Steam Leaderboard Score`** → send your score (and optional `Details[]`).  
4. **`Get Steam Leaderboard Entries`** → choose **Global / Friends / Around User / For Users**.  
5. For each entry, **`Break SAL_LeaderboardEntryRow`** → `SteamID`, `GlobalRank`, `Score`, `Details[]`, `PlayerName`.

---

## **Blueprint Nodes Overview**
**Async**
- `Find Steam Leaderboard` → `OnSuccess(LeaderboardHandle)` / `OnFailure`  
- `Create Steam Leaderboard` → `OnSuccess(LeaderboardHandle)` / `OnFailure`  
- `Upload Steam Leaderboard Score` *(Keep Best / Force Update, optional Details[])* → `OnSuccess` / `OnFailure`  
- `Get Steam Leaderboard Entries` *(Global, Friends, Around User, RangeStart/End, DetailsMax)*  
- `Get Steam Leaderboard Entries (Users)` *(SteamIDs array)*

**Struct**
- `SAL_LeaderboardEntryRow` → `SteamID`, `GlobalRank`, `Score`, `Details[]`, `PlayerName`

**Helpers**
- `Is Steam Available` (bool)  
- `Get SteamID` (from `PlayerController`)  
- `Get Leaderboard Name / Sort Method / Display Type / Entry Count`

---

## **Troubleshooting**
**`IsSteamAvailable = false`**
- Steam client closed  
- `DefaultPlatformService` not set to `Steam`  
- Online Subsystem Steam plugin disabled  
- `steam_appid.txt` missing or misplaced

**Uploaded but not visible**
- Verify you reuse the **same Leaderboard Handle** when downloading  
- Check **Upload Method** (Keep Best vs Force Update)  
- Confirm in Steam overlay (View → Games → View Game Details)

**Friends / Around User empty**
- No prior submissions or Steam privacy/friend filters  
- Test with **Global** first to validate setup

---

## **License**
This project is open source under the **MIT** license. See `LICENSE` for details.

---

<p align="center">
Made with ❤️ by <b>UnForge</b> • If this helped, please ⭐ the repo
</p>
