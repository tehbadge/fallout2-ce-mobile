# Milestone 1A — shared touch UI activation

Historical private-development report: downstream commit references and local evidence below are preserved as technical records, not commits or artifacts available in the consolidated public history. See [current status](STATUS.md#current-accepted-state-2026-10-28).

Initial offline validation on 2026-10-20; physical validation follow-up is recorded below. Implementation was uncommitted at the initial checkpoint on `ios-port`; the user accepted Milestone 1A functionally on 2026-10-20 and authorized its local checkpoint commit. Historical observations below retain their original review context.

## Baseline and changes

Initial working tree was clean. Both requested ancestors were present: `c1f51061dc535ae46dff69037325e8ff06649c9d` (SDL2 lifecycle) and `88f2dd4b77e1777b0a6e4bf786d50de9eb845be1` (guidance).

| File | Change |
| --- | --- |
| `CMakeLists.txt` | Private `FALLOUT_FORCE_TOUCH_ONLY=1` definition on the existing iOS application target. |
| `src/touch.h` | Document the existing switch's iOS use. |
| `src/svga.cc` | Extend Android's existing mobile resolution policy to SDL's `__IPHONEOS__` platform in touch-only mode. |
| `src/game_mouse.cc` | Retain the legacy iOS trackpad override only outside touch-only mode. |
| `src/platform/ios/quick_toolbar.h` | Select existing no-op toolbar functions in touch-only mode. |
| `src/platform/ios/quick_toolbar.cc` | Exclude legacy toolbar implementation under the same guard. |
| `docs/ios-port/STATUS.md` | Current implementation and verification status. |
| `docs/ios-port/MILESTONE-1A.md` | This report. |

`touch.h` already selects `FALLOUT_TOUCH_ONLY` for Android or `FALLOUT_FORCE_TOUCH_ONLY`. This activates existing constant `muiIsEnabled()` / `touchHudIsEnabled()` behavior, native touch processing, and `settingsInit` enforcement of mobile UI, HUD, controls, and world view even with an old desktop/legacy configuration. CMake already includes all shared `mui_*`, HUD, control, and world-view sources; no implementation was copied.

The mobile resolution extension sets 480 logical lines, at least 640 columns with the display aspect ratio, scale 1, and fullscreen. It avoids retaining a desktop-sized/4:3 game canvas on an iPhone. Simulator output was 2622×1206 with a 1043×480 logical game canvas. This reuses the existing policy without adding iOS density or safe-area calculations or moving UI elements.

## Android conditions reviewed

Only the resolution condition needed extension. All other Android-specific facilities retain their original guards:

- Density and cutout JNI in `hud_layout.cc`: unchanged; iOS uses density 1.0 and zero insets pending 1B.
- Accessibility long-press JNI in `touch.cc`: unchanged; iOS keeps the existing 500 ms fallback.
- Import/export activity in `mui_preferences.cc`: unchanged and unavailable in ordinary iOS sessions. No native file workflow added.
- Android internal save-metadata location in `loadsave.cc`: unchanged; iOS keeps relative `ce-*` metadata alongside game data in Documents.
- Android startup storage and back-button hints in `win32.cc`: unchanged. Existing iOS Documents startup and synthetic-mouse suppression remain intact.
- Android-only defaults in `settings.h`: unchanged; shared touch-only enforcement in `settings.cc` supplies the required active settings.

Legacy iOS mouse-emulation helpers in `mouse.cc` are already excluded by `FALLOUT_TOUCH_ONLY`; the legacy character selector returns to its mobile replacement before its iOS adjustments. No changes were needed there.

## Build and integrity results

Xcode 27.1 (27A9275), SDKs 27.1:

- iPhoneOS arm64 Debug, minimum iOS 15.0: **BUILD SUCCEEDED**, unsigned build requested.
- iPhoneSimulator arm64 Debug, minimum iOS 15.0: **BUILD SUCCEEDED**, unsigned build requested.
- Existing macOS arm64 desktop build: passed, including the game target. No desktop runtime testing or Android compilation performed.

Reproducible local commands (existing populated dependency caches, no fetch):

```sh
cmake --preset ios -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_SYSROOT=iphoneos -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build --preset ios-debug -- CODE_SIGNING_ALLOWED=NO
cmake --preset ios -B out/build/ios-simulator -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_SYSROOT=iphonesimulator -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build out/build/ios-simulator --config Debug -- CODE_SIGNING_ALLOWED=NO
cmake -S . -B out/build/macos-scene-check -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build out/build/macos-scene-check --parallel 6
```

Both iOS configurations passed the existing SDL2 revision/full-patch validation. Patch, patch script, SDL dependency configuration, source scene plist, iOS paths, and startup file are byte-for-byte unchanged from Milestone 0. Both built plists passed `plutil -lint`, retained minimum OS 15.0, `SDLPrimaryScene`, `SDLUIKitSceneDelegate`, and disabled multiple scenes. Binary symbols include the scene delegate, shared mobile menu, HUD initialization, native gesture processor, and world-view renderer. Toolbar API symbols may remain as inline no-op stubs; the legacy implementation is excluded.

Generated iOS app configurations contain the private touch definition; the desktop game's compile flags do not. Preprocessor checks produce touch-only 1 for iOS with the definition, 0 for ordinary desktop, and 1 with the Android guard. Android still takes exactly the original resolution branch. These are source/build isolation checks, not an Android runtime claim.

Builds required sandbox escalation for Xcode caches/services. Warnings remain in existing engine/dependency code, plus stale signing-artifact warnings from the reused iPhoneOS output directory. No signing configuration was edited and these build products must not be used as a signed deployment package.

## Simulator runtime and evidence

Simulator: iPhone 18 Pro, iOS 27.0, UUID `E2AC7204-780F-4E91-BBC5-88E1BB78016B`. Automatic approval review initially blocked simulator installation; the user then explicitly approved simulator-only installation. The existing simulator app was updated without uninstalling; its initially empty game-data directory was preserved. Absent assets were copied from the mounted GOG installation, with overwrite refusal, and the current locally generated `ce.dat` was added. This follows the existing iOS manual data-transfer workflow; `ce.dat` supplies the shared UI font and resources. No assets or screenshots were added to Git.

Observed results:

- Initial no-data launch reached the expected missing-master-data dialog, not a scene-lifecycle crash.
- With data, engine startup completed and the opening movie decoded/rendered.
- Existing `ui` scenario with `[ui]skip_opening_movies=2 --dev-new-game` rendered the map/HUD, found the Quick Load control, opened its shared confirmation, dismissed it, repeated with a quick synthetic tap, and finished. All player-tile coordinate checks in that run passed.
- Existing `newgameelder` scenario with intro skip rendered the shared title menu and character selector, found/tapped New Game and Play, and decoded the Elder movie. Screenshots demonstrate the redesigned controls rather than legacy windows.
- Existing UIKit appearance-transition warnings remain nonfatal.

Test limitations: the first `ui` attempt omitted intro skip, so it consumed its steps during opening movies and could not find the HUD; it is not a passing UI test. The broader `mainmenu` scenario stopped at its required Continue-widget check because this fresh simulator has no prior saved session; only its title rendering and picture-resource check passed. Menu/movie screenshots log a player-tile mismatch because no map/player is loaded at those stages; this is not evidence of a gameplay coordinate failure. At that offline checkpoint, full settings, inventory, combat, save/load, background/resume, pinch/pan, physical touch, and physical-device runtime were unverified; subsequent physical results are recorded below.

Local ignored evidence is under `out/`:

- `m1a-ios-configure.log`, `m1a-ios-build.log`, `m1a-simulator-configure.log`, `m1a-simulator-build.log`, `m1a-desktop-configure.log`, `m1a-desktop-build.log`.
- `m1a-simulator-*-console.log`: simulator console output.
- `m1a-evidence/m1a-ui-no-intro/`: completed smoke-test log and HUD/dialog screenshots (`u00_map.png`, `u01_quick_load_dialog.png`, `u05_closed.png`).
- `m1a-evidence/m1a-newgame/`: title, selector, and movie screenshots and log.
- `m1a-evidence/m1a-mainmenu/`: partial title-menu scenario and missing-Continue result.
- `m1a-evidence/m1a-ui/`: unsuccessful intro-timing attempt, retained for transparency.
- `m1a-simulator-startup.png`: expected missing-data dialog before asset preparation.

## Issues and next review

HUD buttons/text and title-menu controls are visibly small on the 2622×1206 output: density remains the non-Android fallback. The starting map has black side regions; do not infer a safe-area or camera-layout fix from initial activation. Insets are zero, so notch/home-indicator clearance is not guaranteed. Synthetic touch routing and the map's coordinate round-trip passed the narrow smoke case; edge gestures, long press, pan, pinch, and real finger usability need measurement.

Review these offline results before separately authorizing a device update. For Milestone 1B, measure drawable/window/logical coordinates, iOS density, both landscape safe areas, and gesture behavior; then make the smallest shared-metrics adaptation. Retest representative menus, gameplay, combat, save/load, audio, and lifecycle on the physical device after approval. Preserve the installed development identity `[preserved development bundle ID]`, fresh scene manifest, and Documents contents during any later deployment. Milestone 1B has not begun.

Offline checkpoint scope: six implementation files and two documentation files; no commits, pushes, branch/remote changes, lifecycle changes, storage changes, or physical-device operations at that checkpoint. Generated build/evidence files remain ignored under `out/`. `git diff --check` must remain clean.

## Physical validation follow-up — 2026-10-20

### Change review

Reviewed the complete tracked diff and this report on `ios-port`, HEAD `88f2dd4b77e1777b0a6e4bf786d50de9eb845be1`: seven modified tracked files and this untracked report. No significant implementation defect identified during review. The six implementation changes are listed above; no source changes were made during physical validation.

The private iOS compile definition and touch-only guards leave Android's existing resolution and touch branches intact and ordinary desktop builds outside the new path. Android was not built or run, and desktop runtime was not retested. Prior iPhoneOS/simulator/macOS compilation results are recorded above. SDL2 lifecycle patch, patch tooling, source scene manifest, Documents startup/path code, and save implementation are unchanged. These establish source isolation, not complete runtime regression coverage.

Retain this report: its reproduction commands, platform-condition audit, evidence paths, and limitations of individual simulator scenarios are durable details beyond the concise STATUS.md summary.

### Device preparation

Xcode/CoreDevice recognizes the paired, wired iPhone 17 Pro Max on iOS 27.0.1 with Developer Mode enabled. The existing app is accessible under `[preserved development bundle ID]`. Documents inventory before deployment contains 106 entries and 582,316,074 file bytes, including GOG data and existing save slots. Local evidence and the Documents backup remain ignored under `out/`.

The current Debug iPhoneOS arm64 build targeting iOS 15.0 passed with the established Personal Team automatic development-signing workflow. A fresh copy of the generated Info.plist changes only CFBundleIdentifier; the built effective plist retains the identical complete UIApplicationSceneManifest (SDLPrimaryScene, SDLUIKitSceneDelegate, multiple scenes disabled) and both landscape orientations. Strict signature verification passed; signed application identity matches the existing app. Evidence: `out/m1a-device-build.log`, `out/m1a-device-codesign.txt`, `out/m1a-device-details.json`, and `out/m1a-documents-before.json`.

### Installation and launch

Complete Documents backup succeeded to `out/m1a-device-backup/Documents`: 93 regular files, all sizes checked against the pre-update inventory; local SHA-256 manifest in `out/m1a-backup-sha256.json`. In-place installation succeeded without uninstalling. Before first launch, all 93 Documents files retained identical sizes and modification times (`out/m1a-documents-after-install.json`). This inventory check is not a post-install byte-for-byte readback of the entire container.

Launch succeeded and reached the redesigned main menu, displaying Continue with existing Arroyo/Village session metadata. Physical screenshot `out/m1a-device-first-launch.png` is 2868×1320 landscape. It confirms very small menu labels/buttons on this device; screen capture alone cannot establish physical cutout or home-indicator occlusion. No lifecycle crash observed at launch. Console capture is `out/m1a-device-console.log`; install details are `out/m1a-device-install.log` and `.json`.

Physical gameplay results below are user-reported actual touch tests on this updated device, not inferred from menu rendering, simulator automation, or Milestone 0 coverage.

Additional physical capture: `out/m1a-device-touch-check.png` shows the shared Display settings screen at 100% interface size, with very small labels and controls placed close to screen edges. This is rendering evidence, not a completed settings-interaction test. The console contains the previously known unbalanced UIKit appearance-transition warnings; no fatal error has been observed in the captured startup log.


### Physical test matrix — actual touch, user-reported

All reported passes below carry the user's qualification: controls are much too small even at 150% interface size. These are functional passes, not usability acceptance.

| Case | Physical result |
| --- | --- |
| Main menu / settings navigation | User reached settings and increased interface size to 150%; shared title menu also captured. Settings exit is almost unclickable. |
| New Game / character selection | PASS, explicitly confirmed by user in follow-up. |
| Continue / Load pre-update save | PASS, user explicitly confirmed successfully loading a save that existed before the update. |
| Character movement | PASS, user-reported. |
| Camera two-finger pan/pinch | PASS, user-reported. |
| HUD buttons | PASS functionally, user-reported; sizing/occlusion substantially impairs access. |
| Target selection / long press | PASS, user-reported. |
| Combat attack / end turn | PASS, user-reported. |
| Inventory | PASS, user-reported. |
| Dialogue | PASS, user-reported. |
| Pip-Boy | PASS, user-reported. |
| Save new slot / load | PASS, user-reported; save/map-load activity also present in console. |
| Background / resume | PASS, user-reported. |
| Both landscape orientations | PASS, user-reported. |

### Confirmed usability defects and runtime observations

- User reports all buttons much too small even at the existing 150% interface-size setting. Top-right settings exit is almost unclickable.
- User reports corner buttons almost completely hidden and barely clickable at 150%; middle-side controls are hidden by the Dynamic Island. This confirms physical safe-area obstruction, beyond the earlier screenshot-only risk. Home-indicator obstruction was not separately identified.
- No systematic touch-coordinate offset was reported. Successful movement/gestures/targeting does not constitute a quantitative coordinate calibration.
- Physical screenshots show shared UI rendering at 2868×1320. No additional rendering corruption was reported; the main confirmed problems are sizing and unsafe edge placement. No geometry or sizing fix was implemented.
- Console retains known nonfatal UIKit unbalanced appearance-transition warnings. One `Error: Couldn't find LoadSave Message!` appears immediately after a completed load (`Total load data read: 62535 bytes`), with subsequent gameplay/save activity. Existing `loadsave.cc` emits this text for missing display messages (including the post-load message); without instrumentation the exact call site is not proven. Record as a nonfatal missing-message diagnostic, not evidence that save/load failed. No crash or new gameplay functional failure was reported.

### Evidence and acceptance assessment

`out/m1a-device-gameplay-check.png` captures Game settings, not gameplay; filenames do not establish coverage. `out/m1a-device-test-progress.png` captures Display settings at 150%. `out/m1a-device-settings-obstruction.png` was requested after the user report but captures the current Arroyo gameplay/HUD, not settings; it confirms active physical gameplay and controls placed at screen edges. Earlier Display screenshots show the top-right settings exit. Screen capture does not include the physical Dynamic Island mask; occlusion confirmation comes from the user's on-device observation.

Retain this report for the detailed audit, reproduction, evidence, and limitations. STATUS.md remains the concise current summary. No source edits, commits, pushes, lifecycle changes, or Milestone 1B implementation were made during physical validation. Git remains on `ios-port` at `88f2dd4b77e1777b0a6e4bf786d50de9eb845be1`, with seven modified tracked files and this untracked report.

Shared touch UI activation and all requested physical functionality tests pass by user report, including explicit follow-up confirmation of New Game/character selection and loading a pre-update save. Milestone 1A is ready for functional acceptance within its activation-only scope. Severe sizing/safe-area limitations remain deferred Milestone 1B work under the requested scope; the installed app is not yet comfortably usable. Physical validation is complete. No 1B implementation was performed.
