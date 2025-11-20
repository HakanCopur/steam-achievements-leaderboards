<h1 align="center"><b>SteamSAL — Steam Achievements & Leaderboards for UE5</b></h1>

<p align="center">
  <img src="Resources/Icon128.png" alt="SteamSAL Icon" width="96" height="96" />
</p>

<p align="center">
  Blueprint-first, async-driven, tiny API surface.<br/>
  Supports Unreal Engine <b>5.0 → 5.6</b> on <b>Win64</b>.
</p>

---

## **New in v1.2.0**

- 🧾 **UGC-enabled leaderboards**
  - Upload and attach arbitrary binary data (replays, ghosts, screenshots, JSON, etc.) to leaderboard scores via **Upload Steam Leaderboard Score With UGC**.
  - Download UGC metadata together with leaderboard rows and check **Has UGC / UGCHandle** from **Get Downloaded Leaderboard Entry**.
- 👥 **Download entries for specific users**
  - Use **Download Steam Leaderboard Entries (Users)** with an array of SteamIDs to fetch only the players you care about (e.g. party members, rivals, or friends list).
- 🧮 **Single-entry struct flow**
  - **Download Steam Leaderboard Entries** now works with a compact **Entries Data** handle and **Get Downloaded Leaderboard Entry** returns a single, strongly-typed struct per index (SteamID, Global Rank, Score, Player Name, Details[], HasUGC, UGCHandle).
- 🧰 **New helper functions**
  - `Format Leaderboard Score` (numeric or time-style formatting),
  - `Get Steam Server Real Time` (Unix + UTC string),
  - `String To Bytes (UTF8)` / `Bytes To String (UTF8)`,
  - `Save Bytes To File` / `Load File To Bytes` for working with UGC payloads and other binary data.

---

## **Table of Contents**
- [**New in v1.2.0**](#new-in-v120)
- [**Table of Contents**](#table-of-contents)
- [**About**](#about)
- [**Features**](#features)
  - [**Leaderboards**](#leaderboards)
  - [**Achievements \& Stats**](#achievements--stats)
- [**Compatibility**](#compatibility)
- [**Install**](#install)
- [**Enable Steam Online Subsystem (Required)**](#enable-steam-online-subsystem-required)
- [**Quick Start — Leaderboards**](#quick-start--leaderboards)
- [**Quick Start — Achievements \& Stats**](#quick-start--achievements--stats)
- [**Blueprint Nodes Overview (Highlights)**](#blueprint-nodes-overview-highlights)
  - [**Async**](#async)
  - [**Pure / Helpers (5+ practical calls)**](#pure--helpers-5-practical-calls)
- [**Troubleshooting**](#troubleshooting)
- [**License**](#license)

---

## **About**
**SteamSAL** is a lightweight Unreal Engine 5 plugin that exposes **Steam Leaderboards, Achievements, and Stats** through simple **async Blueprint** nodes—ideal for solo devs and small teams who want the essentials of Steamworks without writing C++.

---

## **Features**
### **Leaderboards**
- 🔎 **Find / Create** leaderboards
- ⬆️ **Upload** scores (Keep Best / Force Update) with optional `Details[]`
- ⬆️ **Upload scores with UGC** — attach custom binary payloads (bytes) to each score using a UGC file name + data
- ⬇️ **Download** entries (Global, Friends, Around User, For Users, By SteamID list)
- 🧩 **Break** rows → SteamID, Global Rank, Score, Details[], Player Name, **HasUGC**, **UGCHandle**
- 🔢 **Format** leaderboard scores into human-readable text (`Numeric`, `TimeSeconds`, etc.)
- 🧰 Helpers for name, sort, display, entry count, and UGC-related workflows

### **Achievements & Stats**
- 🚀 **Request Current Stats & Achievements** (async) at startup
- 🏆 **Unlock / Clear** achievements; **Indicate Achievement Progress** (toast/progress)
- 📊 **Local (cached) stats**: Set / Add / Batch write → then **Store** to persist
- 🌎 **Global stats**: Request N-day history, aggregate & time-series queries
- 🎛️ UI helpers: **Show Achievements Overlay**, list achievement API names, get display name/icon

---

## **Compatibility**
- **Engine:** UE **5.0 → 5.6**
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
3. **Enable Steam Online Subsystem** — required (see next section).

---

## **Enable Steam Online Subsystem (Required)**
Follow Epic’s official guide (**select your exact UE version** in the doc’s version switcher):  
**▶ Online Subsystem Steam:** https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-steam-interface-in-unreal-engine

**Minimal `Config/DefaultEngine.ini` (UE 5.0–5.6):**
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

## **Quick Start — Leaderboards**
1. Call **`Is Steam Available`** → proceed only if `true`.
2. **`Find Steam Leaderboard`** (or **`Create Steam Leaderboard`**) → cache the **Leaderboard Handle**.
3. For uploading scores, use:
   - **`Upload Steam Leaderboard Score`** for a standard score (+ optional `Details[]`), or  
   - **`Upload Steam Leaderboard Score With UGC`** if you also want to attach UGC bytes (file name + data).
4. For downloading scores, use **`Download Steam Leaderboard Entries`** (Global / Friends / Around User / For Users) or **`Download Steam Leaderboard Entries (Users)`** with a list of SteamIDs.
5. To read a specific row, call **`Get Downloaded Leaderboard Entry`** with an index → `SteamID`, `GlobalRank`, `Score`, `Details[]`, `PlayerName`, `HasUGC`, `UGCHandle`.

---

## **Quick Start — Achievements & Stats**
> Achievements rely on Steam **stats** under the hood. Read/update stats locally, then **Store** to persist to Steam.

1. **Initialize**
   - Call **`Request Current Stats And Achievements`** (async) on game start; wait for `On Success`.
2. **Unlock an Achievement**
   - Call **`Set Achievement (Unlock)`** with the `Achievement APIName`.
   - Call **`Store User Stats And Achievements`** to persist.
3. **Show Progress (incremental achievements)**
   - Option A: Call **`Indicate Achievement Progress`** with `Current Progress` + `Max Progress` (shows Steam toast).
   - Option B: Update a numeric stat with **`Set Local (Cached) Stat`** or **`Add To Local (Cached) Stat`** (e.g., `ST_JUMPS`), then **`Store User Stats And Achievements`**. Drive unlock logic from your stat thresholds.
4. **UI / Overlay**
   - Call **`Show Achievements Overlay`** to open Steam’s achievements page.

**Common pure helpers you can use right away:**
- `Get Achievement API Names` (list)  
- `Get Achievement Display Name` (localized)  
- `Get Achievement Icon` (texture)  
- `Get Global Achievement Percent`  
- `Get Local (Cached) Stat` / `Get Global Stat (Aggregated)` / `Get Global Stat History`

---

## **Blueprint Nodes Overview (Highlights)**
### **Async**
- **Leaderboards:**  
  `Find Steam Leaderboard`, `Create Steam Leaderboard`,  
  `Upload Steam Leaderboard Score`, `Upload Steam Leaderboard Score With UGC`,  
  `Download Steam Leaderboard Entries`, `Download Steam Leaderboard Entries (Users)`,  
  `Get Downloaded Leaderboard Entry`
- **Achievements/Stats:** `Request Current Stats And Achievements`, `Store User Stats And Achievements`, `Request Global Stats`
- **UI:** `Show Achievements Overlay`

### **Pure / Helpers (5+ practical calls)**
- **Core:** `Is Steam Available`, `Get SteamID`
- **Achievements/Stats:** `Get Achievement API Names`, `Get Achievement Display Name`, `Get Achievement Icon`, `Get Global Achievement Percent`,  
  `Get Local (Cached) Stat`, `Get Global Stat (Aggregated)`, `Get Global Stat History`
- **Leaderboards:** `Get Leaderboard Name`, `Get Leaderboard Sort Method`, `Get Leaderboard Display Type`, `Get Leaderboard Entry Count`,  
  `Format Leaderboard Score`
- **Steam / UGC utilities:** `Get Steam Server Real Time`, `Get Steam Display Name From SteamID`,  
  `String To Bytes (UTF8)`, `Bytes To String (UTF8)`,  
  `Save Bytes To File`, `Load File To Bytes`
- **Struct makers:** `Make SteamStat`, `Make SAL_StatWrite` (for batch/local writes)

---

## **Troubleshooting**
**`IsSteamAvailable = false`**
- Steam client closed
- `DefaultPlatformService` not set to `Steam`
- Online Subsystem Steam plugin disabled
- `steam_appid.txt` missing or misplaced

**Uploaded but not visible**
- Reuse the same **Leaderboard Handle** when downloading
- Check **Upload Method** (Keep Best vs Force Update)
- Validate via Steam overlay (View → Games → View Game Details)

**Achievements not unlocking**
- Ensure you called **Request Current Stats And Achievements** before writing
- Call **Store User Stats And Achievements** after updates
- Verify the `Achievement APIName` matches your Steamworks configuration

**Friends / Around User empty**
- No prior submissions, privacy, or friend visibility
- Test **Global** first to validate setup

---

## **License**
This project is open source under the **MIT** license. See `LICENSE` for details.

---

<p align="center">
Made with ❤️ by <b>UnForge</b> • If this helped, please ⭐ the repo
</p>
