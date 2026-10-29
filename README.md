# Fallout 2 CE Mobile (unofficial)

**⚠️ Important: There is no ready-to-install iOS version of Fallout 2 available for download from this project.**

This repository contains the source code for an iOS port. Running it requires building and signing the application yourself using Xcode. A legally acquired copy of Fallout 2 is also required to supply the original game files. The project does not distribute the original game.

Unsigned iOS build artifacts may be available through development workflows, but these are not ready-to-install releases.

This repository is the published unofficial downstream source fork, [tehbadge/fallout2-ce-mobile](https://github.com/tehbadge/fallout2-ce-mobile), of [imbaner/fallout2-ce-mobile](https://github.com/imbaner/fallout2-ce-mobile), itself a fork of [Fallout 2: Community Engine](https://github.com/fallout2-ce/fallout2-ce). imbaner created the Android mobile port and redesigned touch interface. This downstream adds native iOS support and additional fixes while retaining the shared C++/SDL2 engine and mobile UI. **Early beta; public-source publication is complete. No installable iOS binary release has been approved.**

This is an **unofficial, fan-made, modified version** of Fallout 2 CE. It is not affiliated with or endorsed by Bethesda Softworks, ZeniMax Media or Interplay. Fallout is a trademark of its owners. Original Fallout 2 installation archives, music and videos are not supplied. The project includes additional interface resources, fonts and scripts; see [License and attribution](#license-and-attribution). You need a legally acquired copy of Fallout 2 ([GOG](https://www.gog.com/game/fallout_2), [Steam](https://store.steampowered.com/app/38410), [Epic Games](https://store.epicgames.com/p/fallout-2)).

## Screenshots

<p><img src="docs/screenshots/combat.jpg" alt="Combat: an enemy picked, its hit chance" width="49%"> <img src="docs/screenshots/dialogue.jpg" alt="Dialogue: the drill sergeant at Navarro" width="49%"></p>

<p><img src="docs/screenshots/inventory.jpg" alt="Inventory: an item's action menu" width="49%"> <img src="docs/screenshots/character.jpg" alt="Character" width="49%"></p>

## What is changed (modified software notice)

The inherited mobile port provides the following features on top of CE; its engine, fixes and sfall compatibility are retained. The iOS adaptation reuses this work:

- **Touch UI.** The game's screens are replaced with screens made for fingers and the phone's resolution: dialogue, barter, inventory, loot, character and level-up, character creation, Pip-Boy (quests, holodisks with their voice, maps, videos, rest), automap, world map and town maps, skills, game menu, settings, save/load, main menu, dialog boxes, called shots. No cursor: tap to act, long press for the action menu, drag to move items, two fingers to zoom the map.
- **HUD.** Buttons anchored to the screen's edges (sized in dp), the attack modes with reload, the message log, end turn/combat.
- **Tactical view in combat** (a button, can be turned off in the settings): figures turn see-through, the hexes you can reach and everyone's hex are drawn on the ground, so a tile behind an enemy can be picked and an enemy chosen by its hex.
- **Mod features built in** where the mods can't run on CE: Inventory Filter's filters and party tabs in loot and barter.
- **Saves.** A save list sorted by when saves were made, quick saves that never overwrite manual ones (their number is a setting), "Continue" in the main menu, saves interrupted midway are restored, optional marks for saves made with other game files or mods.
- **Android app.** The game's files are imported from a folder or any archive (.zip, .7z, .rar, .tar...) copied from a computer, with RPU and other mods and the saves; "Replace game" for new builds and mod updates; export and import of saves.
- **The port's own improvements** are settings (`[enhancements]` in `fallout2.cfg`).
- **iPhone and iPad.** Shared touch controls, display density and safe-area handling, both landscape orientations, and phone/tablet layouts. Settings → Display offers **Game screens** and **Screen layouts**, including Auto choices for windows and roomier layouts where they fit.
- **iOS quick actions.** **Quick Save/Load in Game Menu** moves the HUD quick actions into the Game Menu when enabled; it defaults to Off and is saved with settings. The separate **Quick saves** setting controls automatic quicksave slots; Off uses the save picker.
- **iOS CRT display.** Settings → Display → **CRT** offers **Off**, **Tonytron**, **Arcade** and **Consumer TV**. Selection changes immediately, survives same-session navigation/background-resume, and is independent of Apply/Reset. It is not saved: each new process starts Off.

The public history preserves upstream commits and consolidates the accepted downstream implementation into one commit; historical private checkpoints and validation are described in [iOS status](docs/ios-port/STATUS.md). iPhone 17 Pro Max and iPad Air 3 have historical physical gameplay acceptance, including save/load, with scope recorded in [iOS status](docs/ios-port/STATUS.md). All 15 hosted host regressions and unsigned iOS arm64 build/package jobs passed at the historical private baseline; these are private CI results, not public workflow runs. The final integrated texture-cache correction still needs physical acceptance before an accepted binary release; this is not testing of every compatible device or OS version.

## Install on Android

1. Download the original mobile maintainer's APK from [imbaner’s releases](https://github.com/imbaner/fallout2-ce-mobile/releases) and install it. These are upstream Android releases, not downstream iOS builds.
2. Copy your Fallout 2 folder from a computer to the phone: as is, or as one archive (cable, cloud drive, messenger). Mods installed there (for example [RPU](https://github.com/BGforgeNet/Fallout2_Restoration_Project)) and saves come along.
3. Start the app and choose that folder or archive.

Upstream reports testing on a Redmi 15C (Android 16) and player use on Alldocube tablets. Android 7.0 or newer, arm64. These reports are inherited; this downstream does not claim new Android physical acceptance.

Plain Fallout 2 from GOG, Steam or Epic works too, with or without mods. The game's language is taken from its own files, older translations included. Problems with the original Android release belong in [upstream’s issue tracker](https://github.com/imbaner/fallout2-ce-mobile/issues).

**Recommended setup: Fallout 2 + [Restoration Project (RPU)](https://github.com/BGforgeNet/Fallout2_Restoration_Project) 2.4.34.** This is the upstream mobile port’s recommended/tested setup; it is not a claim of downstream iOS acceptance for every mod configuration. RPU relies on sfall, which this engine reimplements: Fallout 2: Community Engine, the continuation of the original Fallout 2 Community Edition. Install RPU into the game folder on a computer, then copy the folder to the phone.

## Install on iOS / iPadOS

Requires an arm64 iPhone or iPad with **iOS/iPadOS 15.0 or later**. That is the build minimum, not a physical-test matrix. Both landscape orientations are supported.

There is no ready-to-install iOS release from this project. The development workflow defines an **unsigned IPA** package, accompanied by matching `ce.dat` and an example configuration. GitHub Actions are disabled in this public repository; historical private runs and artifacts are not public downloads. It cannot be directly installed as an ordinary signed app. Build and sign with your own Apple development credentials; see [iOS build and resources](CONTRIBUTING.md#ios--ipados). Development-signed Xcode installations were historically tested; AltStore, Sideloadly, TestFlight and App Store distribution have not been validated for this downstream.

After installing a signed build, launch it once to create its Documents directory. Launch alone cannot establish gameplay readiness without game files. Use Finder’s device **Files** tab to copy `master.dat`, `critter.dat`, `patch000.dat` where supplied by your installation, matching `ce.dat`, and the relevant `data` and `sound` folders into the app’s document root, without an extra enclosing Fallout2 folder. The app enables File Sharing and reads game files from Documents. Keep resource paths consistent with your configuration. A complete folder transfer through every Finder/Windows client is not independently validated here; Android’s folder/archive importer is not implemented on iOS.

Use `ce.dat` from the same source revision/build package as the engine. Preserve existing `fallout2.cfg` and `SAVEGAME`; the example configuration is a reference, not an instruction to overwrite them. Back up Documents before updating. Keep the installed bundle identity and compatible signing identity when updating in place: changing identity may create another app/container, and uninstalling may delete saves. Provisioning/trust and renewal requirements depend on your Apple account. The CI bundle identity is not a promise of compatibility with an existing development installation.

Touch controls are shared with the Android mobile interface: tap destinations/actions, long press for context actions, drag items, and use two fingers to pan/pinch the map. HUD and screen buttons provide navigation; the legacy CE cursor/trackpad gesture lists do not describe this touch-only build.

## Known limitations

- Landscape only; no portrait acceptance or downstream browser validation.
- Final integrated texture-cache correction awaits physical acceptance; see [current status](docs/ios-port/STATUS.md#current-accepted-state-2026-10-28).
- Higher-scale iPad usability and broad device/OS compatibility are not exhaustively established. Desktop builds retain CE’s original windows (`[touch] mobile_ui=0`).
- CRT is optional and increases GPU cost. Historical iPad Air 3 profiling measured roughly 47–52 FPS with CRT versus 59 FPS Off in one scene; battery impact remains unmeasured.
- Partial save relocation (F8) and allocation failure during renderer recreation (F9) remain assessed, deferred edge cases. This does not negate ordinary save/load acceptance.
- Mods with custom script windows are not covered by the shared touch UI.

## Reporting problems

Use [downstream Issues](https://github.com/tehbadge/fallout2-ce-mobile/issues) for native iOS problems, shared mobile UI behavior introduced or changed downstream, additional fixes or CRT bugs, and questions about building and running this source. Include engine commit/build, device and OS, reproduction steps, relevant settings and logs with personal data removed. Provide saves only when needed and share them deliberately; do not upload purchased game archives. Original Android-release issues that reproduce on unchanged upstream should generally use [imbaner’s issue tracker](https://github.com/imbaner/fallout2-ce-mobile/issues).

## Building

Android: JDK 17, Android SDK with NDK 27.3.13750724.

```
cd os/android
./gradlew assembleRelease -PabiFilters=arm64-v8a
```

A release build is signed with your own key: point the Gradle property `fallout2ce.releaseKeystoreProperties` to a properties file with `storeFile`, `storePassword`, `keyAlias`, `keyPassword` (never commit it). Desktop builds are as upstream's (CMake). For iOS signing, matching resources and host regression commands, see [CONTRIBUTING.md](CONTRIBUTING.md).

## License and attribution

The engine is under the [Sustainable Use License](LICENSE.md), as upstream: free, non-commercial distribution only, with the license and notices kept. Third-party components: SDL 2, zlib, LodePNG (zlib license), libarchive (BSD), liblzma from XZ Utils (0BSD), stb (MIT / public domain), fpattern, Fallout Classic Dialog font (MIT), PT Mono font (SIL OFL 1.1). Existing component texts are available in Settings → About. That list does not yet cover every inherited component described below.

Credits: Alexander Batalov and the Community Engine contributors for the engine; imbaner for the Android port and redesigned shared touch UI; tehbadge’s downstream development for the iOS adaptation and additional fixes. Thanks to the sfall, RPU and Inventory Filter authors for their work and compatible behavior.

Compatibility implementations and redistributed components are distinct: this repository includes sfall’s compiled `gl_highlighting.int` and matching `barter_e.frm`, `trade_e.frm` and `iface_apbar_e.frm` resources. [sfall’s repository](https://github.com/sfall-team/sfall) carries GPLv3; historical questions about component-specific terms, source/build materials and distribution compatibility remain unresolved. This notice is not a completed GPL compliance package or a relicensing of the engine.

Inherited inventory artwork derives from [FISSION](https://github.com/cambragol/fission-ce) work credited to Golbolco and cambragol. Stock HRP resources have [Mash’s documented permission](https://github.com/rotators/Fo1in2/blob/master/Fallout2/Fallout1in2/f2_res_README.rtf) for noncommercial redistribution/modification with authorship preserved; historical provenance questions concerning modified HRP/Sonora-related resources, selected CE interface art and platform icons remain unresolved. Fonts and other dependencies retain their own notices. This noncommercial hobbyist fork preserves inherited resources, licenses, notices and attribution. Broad investigations of unchanged upstream materials are outside this publication phase; the four proposed maintainer inquiries are canceled. Active diligence covers materials newly introduced or materially modified downstream, without treating historical questions as legally resolved. See [publication status](docs/ios-port/STATUS.md#publication-preparation) for the completed source publication and remaining binary-release work. No installable iOS binary release has been approved.

---

# Fallout 2: Community Engine

The following inherited CE reference documents the engine’s broader capabilities and upstream releases; it is not downstream iOS release acceptance. Mobile-specific instructions above take precedence.

Fallout 2:CE is a fully working re-implementation of the classic Fallout engine, optimized for a hassle-free experience on multiple platforms, including Windows, Mac, iOS, Android, and Linux.  It provides high resolution support, quality-of-life improvements, and dozens upon dozens of bug fixes.

This is a fork of the original Fallout 2:CE project, which is no longer getting regular updates.

Popular total conversion mods are supported:
 * [Fallout Et Tu](https://github.com/rotators/Fo1in2): Full Support.  Use v2.0+
 * [Fallout: Sonora](https://github.com/NovaRain/Fallout-Sonora-English): Full Support.
 * [Fallout 2 Restoration Project](https://github.com/BGforgeNet/Fallout2_Restoration_Project): Full Support.
 * Fallout of Nevada: Supported with minor tweaks needed.
 * [Olympus 2207](https://olympus2207.com), Resurrection, Yesterday: partially work but not fully supported.

CE has broad (though not total) compatibility with [Sfall](https://github.com/sfall-team/sfall) scripting extensions.  Many traditional Fallout mods work out of the box, including NPC Armor, Party Orders, FO2Tweaks (we recommend v14.5+), Talking Heads, and many others.

## Inherited CE installation reference — [Upstream CE releases](https://github.com/fallout2-ce/fallout2-ce/releases)

You *must* own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/fallout_2), [Epic Games](https://store.epicgames.com/p/fallout-2) or [Steam](https://store.steampowered.com/app/38410).

Download CE from the link above, then extract the archive into your game folder. Launch `fallout2-ce.exe` on Windows (or the platform-specific executable) to play. CE has a lot of enhancements that require configuration. See [Configuration](#configuration) for examples.

For detailed instructions on how to play on Mac, Linux, iOS, or Android, see the [Platform-specific instructions](#platform-specific-installation).

## Quality of life benefits over vanilla Fallout

* High resolution support
* Party members can loot and barter in place of PC
* Directly equip party members instead of convincing them to use the right equipment.
* Expanded 2-column inventory, loot screens
* Expanded 4-row barter screen
* Expanded AP bar
* Ctrl-click to quickly move items when bartering, looting, or stealing, and auto-balance caps
* Music continues playing between maps
* Auto open doors
* Integrated "HELP" menu
* 44.1 kHz stereo sound/music supported, in .ogg and .wav format as well as legacy .acm
* Last used save slot is remembered
* You can cancel elevator floor selection using ESC
* Spiffy ammo meter with burst-mode segmentation
* Item/Corpse/Container/Critter highlighting (configure using [mods/sfall-mods.ini](https://github.com/sfall-team/sfall/blob/master/artifacts/config_files/sfall-mods.ini))

<table>
  <tr>
    <td align="center">
    <img src="docs/images/two-column-inventory.jpg" alt="Expanded 2-column inventory" width="360"><br>
    <sub>Expanded 2-column inventory</sub>
    </td>
    <td align="center">
      <img src="docs/images/companion-equip.gif" alt="Companion equip screen" width="360"><br>
      <sub>Companion equip screen</sub>
    </td>

  </tr>
  <tr>
    <td align="center">
      <img src="docs/images/party-member-loot.gif" alt="Party member loot" width="360"><br>
      <sub>Party member loot</sub>
    </td>
    <td align="center">
      <img src="docs/images/help-menu.gif" alt="Integrated HELP menu" width="360"><br>
      <sub>Integrated HELP menu</sub>
    </td>
  </tr>
</table>

There are also dozens of small things that just work a little better than they did in the original.  Better pathfinding, fewer graphics glitches, less finicky weapon stacking, and much more.

## Configuration

The main configuration file is `fallout2.cfg`. This contains original game config as well as new CE settings.

Additional settings for screen resolution, UI customization, and map options are now integrated into the main `fallout2.cfg` file (previously part of `f2_res.ini` from Mash's HRP).

Here are some important settings in `fallout2.cfg` under the `[screen]` and `[ui]` sections.  See [the example config](https://github.com/fallout2-ce/fallout2-ce/tree/refs/heads/main/files/fallout2.cfg) for a full list of settings.

```ini
[screen]
resolution_x=1600 ; actual game window size (screen resolution), in pixels
resolution_y=1080
windowed=1 ; 0 = fullscreen
scale=2 ; 1 = original scale, 2 = 2x scale, etc. (e.g. at scale 2 and screen resolution 1920x1080, in-game resolution will be 960x540, thus every pixel is twice as wide and tall)

[ui]
; Maximum number of columns shown in the main inventory and loot/steal windows (valid range: 1..2)
inventory_columns=2

; Set to 1 to expand the barter/trade window vertically, adding a 4th item slot per side (requires ce.dat)
expand_barter_window=1

[qol]
; Allow opening party member inventory from the long-press context menu.
party_trade_from_menu=1

; Allow switching to companions' inventories in loot screens (and barter, in the future)
party_loot_and_barter=1
```

**Desktop/legacy CE configuration reference:** the mobile touch-only build chooses its game geometry automatically. Use Settings for mobile UI size, screen layouts and display choices rather than these manual sizing examples.

**Recommendations:**

- **Desktops**: Use any size you see fit.
- **Tablets**: Set these values to logical resolution of your device, for example iPad Pro 11 is 1668x2388 (pixels), but its logical resolution is 834x1194 (points).
- **Mobile phones**: Set height to 480, calculate width according to your device screen (aspect) ratio, for example Samsung S21 is 20:9 device, so the width should be 480 * 20 / 9 = 1067.

The mobile Settings screen exposes many of these choices; advanced options remain in `fallout2.cfg`. Running and quitting the game writes supported defaults to the configuration.
*Note*: Legacy IFACE_BAR graphics use optional `f2_res.dat`. The inherited [upstream resource reference](https://github.com/fallout2-ce/fallout2-ce/raw/refs/heads/main/files/f2_res.dat) is not a required iOS download or a statement that all resource permissions are resolved; the current iOS CI ZIP excludes it.

Note: There are a few settings that are still configured in `ddraw.ini`.  They will be migrated to cfg over time.

## Platform-specific installation

### Windows

[Download](https://github.com/fallout2-ce/fallout2-ce/releases) and unzip into your `Fallout2` folder. Launch the game using `fallout2-ce.exe`.

### Linux

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout2` folder somewhere, for example `/home/john/Desktop/Fallout2`.

- Alternatively you can extract the needed files from the GoG installer:

```console
$ sudo apt install innoextract
$ innoextract ~/Downloads/setup_fallout_2_1.02_gog_v1_\(77792\).exe -d Fallout2
```

- Download the Linux release archive, extract `fallout2-ce` and `ce.dat`, and copy them into this folder.

- Run `./fallout2-ce`.

### macOS

> **NOTE**: macOS 10.11 (El Capitan) or higher is required. Runs natively on Intel-based Macs and Apple Silicon.

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout2` folder somewhere, for example `/Applications/Fallout2`.

- Alternatively you can use Fallout 2 from Macplay/The Omni Group as a base - you need to extract game assets from the original bundle. Mount CD/DMG, right click `Fallout 2` -> `Show Package Contents`, navigate to `Contents/Resources`. Copy `GameData` folder somewhere, for example `/Applications/Fallout2`.

- Or if you're a Terminal user and have Homebrew installed you can extract the needed files from the GoG installer.  **Note**: You must use "Offline backup game installer", not the main game installer.

```console
$ brew install innoextract
$ innoextract ~/Downloads/setup_fallout_2_1.02_gog_v1_\(77792\).exe -d fallout2
$ mv fallout2 /Applications/Fallout2
```

- [Download](https://github.com/fallout2-ce/fallout2-ce/releases) and copy `Fallout II Community Edition.app` to this folder.

- Run `Fallout II Community Edition.app`.

### Android

For this mobile fork, use [Install on Android](#install-on-android). Community Engine’s legacy cursor controls and installer are separate from the redesigned mobile UI.

### iOS

For this downstream, use [Install on iOS / iPadOS](#install-on-ios--ipados) and [iOS build instructions](CONTRIBUTING.md#ios--ipados). The shared touch UI replaces the inherited CE trackpad/toolbar controls.

### Browser

> **NOTE**: WebAssembly build with emscripten
```
docker run --rm -v $(pwd):/src emscripten/emsdk:3.1.74 sh -c 'git config --global --add safe.directory "*" && mkdir -p build && cd build && emcmake cmake ../ -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/Emscripten.cmake && emmake make'
```
- Demo available at https://github.com/ololoken/fallout2-ce-ems.git


## Contributing

For build instructions and contributor notes, see [CONTRIBUTING.md](CONTRIBUTING.md).

Integrating Sfall goodies is the top priority. Quality of life updates are OK too.  In any case open up an issue with your suggestion or to notify other people that something is being worked on.

For current sfall compatibility status and the remaining work needed to close gaps, see [SFALL_COMPATIBILITY.md](SFALL_COMPATIBILITY.md).

## For modders

* Robust (though not 100%) Sfall opcode and hook support
* .ogg/.wav support for map music
* .png support for static assets (must be 8bit indexed)
* .zip support for .dat archives
* Working BIS mapper

## License

The source code is this repository is available under the [Sustainable Use License](LICENSE.md).

[Русская версия](README.ru.md)
