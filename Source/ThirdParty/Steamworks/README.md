# Steamworks SDK (Documentation Only)

This plugin calls Valve's Steamworks SDK for Steam achievements, stats, and leaderboards.

- **Name:** Steamworks SDK
- **Vendor:** Valve Corporation
- **Version:** v1.57+ (matches the version bundled with your installed Unreal Engine)
- **License:** Steamworks SDK Access Agreement (proprietary)
- **License URL:** https://partner.steamgames.com

## What is included here?
**No Steamworks SDK source or binaries are redistributed** in this plugin. This folder exists to document the third-party dependency only.

## How the plugin uses Steamworks
- Calls `SteamUserStats` / `SteamUtils` APIs to fetch and submit achievements, stats, and leaderboard data when running on Steam.
- All Steamworks headers/libraries are referenced from the Unreal Engine installation (Engine/Source/ThirdParty/Steamworks).

## Build integration
- The module references the Engine’s Steamworks package:
  - `AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");`
  - `WITH_STEAMWORKS` macro guards direct Steamworks usage in `.cpp` files.
- No Steamworks files are packaged with the plugin.

## Runtime requirements
- The consuming project must enable **Online Subsystem** (and typically **Online Subsystem Steam**).
- A valid **Steam AppID** and Steamworks configuration are required for Steam features.

## Legal notice
Steamworks is provided by Valve Corporation under the Steamworks SDK Access Agreement. This plugin does not redistribute Steamworks.
