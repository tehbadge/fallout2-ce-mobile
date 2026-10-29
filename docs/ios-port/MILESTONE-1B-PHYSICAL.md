# Milestone 1B — physical validation, 2026-10-20

Historical private-development report: downstream commit references and local evidence below are preserved as technical records, not commits or artifacts available in the consolidated public history. See [current status](STATUS.md#current-accepted-state-2026-10-28).

Milestone 1B display geometry and physical functionality were accepted by the user on 2026-10-20, with the minor visual/diagnostic observations below retained for review. No blocking regression found. This report preserves the physical validation evidence; the subsequent checkpoint was explicitly authorized.

## Review and build

Reviewed on `ios-port`, HEAD `8c85e605255e07b0f479646736888449afaf855c`. Read repository AGENTS.md, STATUS.md, the complete tracked diff, and all four new source/test files. No implementation edits were made during physical validation.

- `display_metrics.mm` obtains the SDL UIKit window and attached root-controller view, which SDL sets to its active rendering view. Drawable dimensions come from `SDL_GetRendererOutputSize`; density is measured independently on both axes. UIKit queries stay on the main thread. A noninteractive observer invalidates measurements on layout, safe-area/attachment and lifecycle changes; current scene orientation and drawable-size comparisons refresh the cache as well.
- `display_geometry.h` rounds point insets outward per axis. Logical conversion subtracts the actual viewport origin and opposite-edge bars before dividing by the corresponding SDL scale. Inspection of the pinned SDL `SDL_RenderWindowToLogical` implementation confirms that querying window (0,0) recovers the floating drawable viewport origin without introducing an input-coordinate adjustment.
- iOS helper sources are under CMake `if(IOS)`; `hud_layout.cc` and `mui.cc` use iOS guards. Android JNI/density/inset/cache paths and the original segmented-label minimum remain intact. Desktop behavior is unchanged apart from registering a host geometry test and inactive developer-test logging. No Android build/runtime claim.
- iOS-only constrained segmented labels shrink below the previous minimum; the physical 150% screenshot confirms all four violence labels fit. This intentionally permits smaller choice text in constrained rows; readability remains a tradeoff at high scale.
- Touch, gesture, mouse-coordinate, save/storage-path, and scene lifecycle implementation files are unchanged. No global touch offset was introduced. Settings Back uses the same full rectangle for rendering and `touchable` in the shared `muiTabButton`.
- Host geometry test rebuilt and passed with assertions enabled. Coverage includes unavailable/zero/late-nonzero insets, symmetric/asymmetric landscape edges, both density axes, independent logical scales, asymmetric viewport bars, outward rounding, changed drawable sizes, invalidation/recovery, and nonfinite dimensions. Tests use snapshots; they do not simulate UIKit callbacks directly. Real orientation changes were additionally observed below.
- `git diff --check` passed. Offline iPhoneOS/simulator/macOS results are preserved in STATUS.md; this review reran the host geometry test and signed physical-device build. It did not rerun desktop/simulator runtime tests or build Android.

Fresh offline iOS CMake configuration verified the complete pinned SDL2 scene patch was already applied. Personal Team automatic development signing used the established team and existing identity `[preserved development bundle ID]`. iPhoneOS arm64 Debug build, minimum iOS 15.0: **BUILD SUCCEEDED**. The fresh generated signing plist differs only in bundle ID. Effective signed plist retains the complete scene manifest, SDLPrimaryScene / SDLUIKitSceneDelegate, disabled multiple scenes, and both landscape directions. Scene-delegate and display-helper binary symbols verified. Strict codesign verification passed with macOS trust-service access; the sandbox-only trust check could not establish trust.

## Data preservation and installation

Physical target: Xcode/CoreDevice-connected iPhone 17 Pro Max, iOS 27.0.1.

Complete recoverable backup outside Git: `~/Fallout2-iOS-Backups/m1b-20261020/Documents`. Contains 159 regular files / 582,340,605 bytes, including GOG assets and slots 01–03 (99 save-related files). Every file size matched the device inventory, and a SHA-256 manifest plus original inventory are stored beside the backup. No symlinks or missing files were accepted.

In-place installation succeeded under the existing bundle ID, without uninstalling or erasing the container. Before first launch, all 159 files retained identical sizes and modification times. After launch, all 99 save-related files were read back and matched the backup hashes exactly. Main-menu Continue metadata remained available; the user subsequently passed loading a pre-update save.

After user gameplay/save tests, final inventory retained every original filename and added slot 04 (35 save-related files). `master.dat`, `critter.dat`, `patch000.dat`, and `ce.dat` retained sizes/mtimes. Normal gameplay updated runtime state, configuration and saves; the final Documents tree is not claimed byte-identical to the pre-launch backup. No assets or saves were manually modified or deleted.

## Physical measurements

Values are from live console diagnostics on the physical device, not inferred from the simulator. Insets use left/top/right/bottom order.

| Measurement | Physical iPhone 17 Pro Max | Prior iPhone 18 Pro simulator |
| --- | --- | --- |
| Renderer output | 2868×1320 pixels | 2622×1206 pixels |
| Attached rendering view | 956×440 points | 874×402 points |
| Density X/Y | 3.0000 / 3.0000 | 3.0000 / 3.0000 |
| Safe insets, points | 62 / 0 / 62 / 20 | 62 / 0 / 62 / 20 |
| Converted output insets | 186 / 0 / 186 / 60 pixels | 186 / 0 / 186 / 60 pixels |
| Output safe rectangle (x,y,w,h) | 186, 0, 2496, 1260 | 186, 0, 2250, 1146 |
| Observed orientation | 3 and 4, both landscape directions | 3, landscape left |
| UI scale / effective menu density | 100% → 3.0; 150% → 4.5 | 100% → 3.0; 150% → 4.5 |

Startup and repeated orientation 3 → 4 → 3 snapshots are valid, including 180-degree changes with unchanged dimensions/insets. Later valid snapshots continued during user-assisted lifecycle tests. PID 1711 remained the running process after background/resume and lock/unlock. Log entries do not label each individual user action, so callback timing for each action was not independently instrumented. User confirms rendering/input survived both actions with no bugs noted.

Configuration readbacks confirm scale changes were saved: `out/m1b-device-current.cfg` and `out/m1b-device-final.cfg` contain 100%; `out/m1b-device-scale150.cfg` contains 150%. Automatic density remains enabled. No forced process restart was performed solely to test preference persistence.

## Actual-touch results

These are the user's physical tests on the newly installed build, not simulator inference or Milestone 1A carryover. In response to the explicit four-run question the user confirmed all Back taps worked, with no home-indicator issues.

| Settings Back test | Result |
| --- | --- |
| 100%, first landscape direction | 10/10 |
| 150%, first landscape direction | 10/10 |
| 100%, opposite landscape direction | 10/10 |
| 150%, opposite landscape direction | 10/10 |

The user confirmed controls safely clear the Dynamic Island and corners and click as expected. No systematic touch offset or home-indicator obstruction was reported.

| Physical case | Result / basis |
| --- | --- |
| Movement | PASS, user report |
| Map pan/pinch | PASS, user report |
| Long press / targeting | PASS, user report |
| Combat | PASS, user report |
| Inventory / dialogue / Pip-Boy | PASS, user report |
| Save in a new slot / load it | PASS, user report; slot 04 and completed save/load logged |
| Load pre-update save | PASS, explicit user report |
| Background / resume | PASS, user report; same process remains active |
| Lock / unlock | PASS, user report; same process remains active |
| Both landscape directions | Valid geometry logged; user Back tests pass in each |
| Clipping, excessive sizing, scrolling, touch offsets, unexpected restarts | No bugs noted in response to the explicit usability/regression question |

No exhaustive screenshot matrix of every modal, inventory, dialogue, Pip-Boy, combat state, scale and orientation was collected. User testing covers those functional screens; visual inspection evidence is limited to the menu, Settings, and noncombat HUD captures below. Radial-menu selection was not separately reported beyond long-press/targeting.

## Screenshots and observations

All physical captures are 2868×1320 PNGs and remain ignored under `out/`.

| Local capture | What it establishes |
| --- | --- |
| `out/m1b-device-first-launch.png` | Main menu at persisted 150%; Continue metadata; larger controls; copyright text overlaps the lower Intro/Credits/Exit row |
| `out/m1b-device-settings-user-check.png` | Display settings showing 100%; safe Back and bottom actions; no clipping observed |
| `out/m1b-device-settings150.png` | Game settings at saved 150%; all violence labels fit; safe Back and bottom actions; smaller choice text in constrained row |
| `out/m1b-device-final-settings.png` | Actually gameplay at saved 100%; noncombat HUD safe edge placement, no group overlap; filename does not imply Settings coverage |
| `out/m1b-device-hud150.png` | Gameplay HUD at verified saved 150%; larger controls and weapon panel occupy more of the map, with no visible group overlap or clipping |
| `out/m1b-device-test-progress.png` | Earlier gameplay/HUD capture, scale not independently established at capture time |

The formerly obstructed gameplay HUD is represented at both scales. Physical cutout/home-indicator clearance is established by the user's observation because screenshots omit the hardware mask. Main menu was captured at 150% only. The 150% weapon panel occupies substantial map area, as expected from scaling; the user reported no resulting usability bug.

## Remaining defects and diagnostics, ranked

1. **P3 — main-menu visual overlap at 150%.** Copyright text crosses the lower menu button row in the physical startup capture. Nonblocking; no source correction made during validation.
2. **P3 — dialogue transition diagnostic.** Console logged one `Error locking transition...` during a talking-head art transition. Unchanged `game_dialog.cc` emits this when `artLock` cannot obtain transition art. Dialogue continued afterward and the user passed dialogue testing. This occurrence was not found in the previous 1A console; its cause is not proven to be new or related to geometry. Retain for review without expanding this milestone.
3. **Known nonfatal UIKit appearance warnings.** The same unbalanced appearance-transition messages observed before 1B recur at startup. No startup crash or lifecycle regression observed. No SDL lifecycle fix attempted.

No P0–P2 defect identified in this validation. Smaller fitted labels at 150% and reduced map visibility are documented tradeoffs, not user-reported failures. No additional gameplay/gesture failure was reported.

## Git state and evidence

At the end of physical validation, branch and HEAD were unchanged and implementation was uncommitted. Physical validation changed documentation only; no pushes, remote changes, new branches, merge/rebase, import functionality, lifecycle changes, or Milestone 1C work occurred. The subsequent user-authorized local checkpoint records acceptance in STATUS.md.

Milestone worktree: modified `CMakeLists.txt`, `src/dev_autotest.cc`, `src/hud_layout.cc`, `src/mui.cc`, `docs/ios-port/STATUS.md`; new `src/platform/ios/display_geometry.h`, `display_metrics.h`, `display_metrics.mm`, `tests/ios_display_geometry_test.cc`, and this report. No generated files, game assets, signing credentials/profiles, or backups added to Git.

Additional ignored evidence: `out/m1b-physical-configure.log`, `out/m1b-device-build.log`, `out/m1b-device-{install,launch}.json`, `out/m1b-device-console.log`, `out/m1b-documents-{before,after-install,final}.json`, `out/m1b-backup-sha256.json`, and `out/m1b-save-readback/`. External backup includes its own inventory and hash manifest. Tests establish the specified device functionality; they do not establish Android runtime behavior or every iOS edge case.
