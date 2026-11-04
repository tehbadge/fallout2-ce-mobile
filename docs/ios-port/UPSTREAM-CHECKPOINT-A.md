# Upstream integration — Checkpoint A (2026-11-03)

Local implementation and automated verification only. MANAGE review and Checkpoint B runtime acceptance remain separate. No publication or deployment is authorized by this checkpoint.

## Verified baseline and target

- Canonical public downstream: `tehbadge/fallout2-ce-mobile`, clean `main` and remote `main` at `a919da9c0c82bfe178c4cba490123ca3bd42dd49`, tracking `origin/main`.
- Original public upstream: `imbaner/fallout2-ce-mobile`, fetched `main` at `577c6d96523458f1b4e1f8970835ba30a93feccc`. This exact target was frozen before implementation; no later commits were chased or omitted from the observed delta.
- Shared integrated ancestor: `8c11b9a3d9ab1c7f3d7ab5a4e84fc2cffca0a7a8`; 168 incoming upstream commits, 25 overlapping paths.
- Historical private repository: `tehbadge/fallout2-ce-mobile-ios`, clean local `ios-port` and remote `ios-port` at `9836a20413adbfa09d5104769bfa4a061c7cfb37`. It was inspected, not used as a baseline.
- Repository API metadata independently confirmed both source repositories public and the historical repository private. URLs, branch heads, tracking, working trees and worktree registrations were inspected before implementation.

The complete seven-commit delta beyond reviewed `b0460ecea54ad1a542531e82ffcbb47a73fe99e8` is included:

| Commit | Assessment |
| --- | --- |
| `17e1a2e06554203031bad400a94e583b6a2b8b9a` | Web CI mixer reference uses frame-wise conversion and the web SDL release; no native dependency change. |
| `cad45f902cdadb6a0dc12db8035b1bbb23637e80` | Shared autotest scenario table avoids MSVC nesting limits; downstream display-geometry scenario retained. |
| `5568fedba9d71461422c274fe243752d9e6d3744` | Shared autotest lambda constant gets static storage for MSVC. |
| `e1db791e8b36005160e525a97f022c0e5f62658a` | Shared app-version helper registered in CMake; native compiled version/hash retained, browser receives version/build from its page. |
| `02e713b63b12aa17b38a5f7e4c3ab1d48fa25ea5` | Browser service-worker reuse, progress display and idle update handling; hash/size checks retained. |
| `c2aafb6675a877b51ea3d47fd60776810d7e1fdb` | Web-only Emscripten cache and ports paths. |
| `577c6d96523458f1b4e1f8970835ba30a93feccc` | Web update documentation. |

No new architectural/platform decision or major native dependency change was found in those seven commits. Source inspection covered all 18 changed paths, including CMake, autotests, version/report/settings code and web build/runtime configuration.

At the frozen SHA, upstream [web CI passed](https://github.com/imbaner/fallout2-ce-mobile/actions/runs/41681646984). Upstream [native Build](https://github.com/imbaner/fallout2-ce-mobile/actions/runs/41681647213) passed macOS, Linux, Android, Windows x86/x64, static analysis, formatting and resource generation; iOS was cancelled. Its check annotation states: “The job was not acquired by Runner of type hosted even after multiple attempts”. This is runner-capacity evidence, not an iOS compiler failure. Local builds below independently verify this integration.

## Merge and conflict resolutions

Branch: `codex/upstream-checkpoint-a`. Worktree: `/private/tmp/fallout2-upstream-checkpoint-a`.

A non-destructive `git merge-tree --write-tree main upstream/main` preview preceded the normal `--no-commit --no-ff` merge. Both preview and actual merge found nine conflicted files. The final local merge commit has downstream `a919da9c` as first parent and upstream `577c6d96` as second parent; its exact hash is supplied with the final handoff (`git show -s --format='%H %P' HEAD`). All adaptations are included in that merge, with no separate adaptation commit.

| Conflicted file | Resolution |
| --- | --- |
| `.github/workflows/ci-build.yml` | Retained canonical Balanced workflow byte-for-byte: three jobs, triggers/exclusions, read-only permissions, unsigned iOS, no uploads/releases. |
| `README.md` | Retained downstream identity, native iOS installation/support/acceptance limits; incorporated shared feature and web documentation, explicitly identifying upstream channels. |
| `README.ru.md` | Same downstream distinctions and upstream attribution in the existing translation. |
| `src/dev_autotest.cc` | Retained new scenario table and compiler fixes plus downstream geometry scenario and drag-offset assistance; positive `b` remains upstream target-height fraction. |
| `src/hud_layout.cc` | Kept iOS density/geometry handling and added distinct web branches. |
| `src/mainmenu.cc` | Adopted typed upstream menu API while preserving iOS Exit shortcut suppression and explicit engine shutdown handling. |
| `src/mui.cc` | Adopted shared tab-row layout with the iOS text-fitting floor, retaining downstream input/scroll handling. |
| `src/mui_main_menu.cc` | Combined typed choices and optional update row with clipped, scrollable full-size controls, viewport input gating and iOS Exit suppression. |
| `src/mui_preferences.cc` | Retained upstream centralized Apply/rollback, new highlighting and scroll-limit fields, and downstream persistent quick actions and session-only CRT. |

## Functional integration and scope of evidence

- **Highlighting:** iOS containers/corpses default On; hidden objects default Off. All three remain editable, resettable and persistent through the registry. Apply rollback restores them and quick actions together. Other platforms retain upstream defaults. Source inspection confirms script-hidden objects remain excluded and mod colors, exclusions and line-of-sight logic remain in the shared implementation.
- **Hero Appearance:** upstream implementation remains mod-gated, including game.cfg/ddraw.ini migration, mod race/style sets, creation controls, script selection, save/character-file state, fallback art and cache flushes. Ordinary games remain disabled. No separate iOS creation redesign. Functional gameplay and mod acceptance remain for B.
- **Touch/map:** independent map/HUD finger ownership is exercised in production MUI code; normal and iOS harnesses retain slider/scroll arbitration. A production-function harness checks all 16 engine-wait/script-lock/map-scroll combinations. Script-controlled UI locks remain restrictive.
- **Tactical targeting:** upstream noncritter interaction and highlight-assisted pickup targeting retained. Inspected and compiled; game-world interaction remains for B.
- **Tabs/fades:** shared mobile tab layout and upstream palette-fade visibility correction retained. Geometry, hit-testing and clipping have automated coverage; visual gameplay acceptance remains for B.
- **Saves:** upstream serialization filtering and legacy recovery coexist with downstream save-catalog identity/crash-recovery assertions. Oldest-first physical relocation is explicitly asserted in addition to retained identity, chronology, composition metadata, retention and recovery checks. Tests use disposable fixtures only.
- **Art/F2:** upstream art-cache generation joins the downstream palette/upload validity checks. Tests exercise same-fid generation changes, failed-upload retry, cache reuse, screen lifetime and renderer reset, including ASan+UBSan.
- **iOS rendering/lifecycle/CRT:** native Metal renderer, SDL scene-lifecycle and snapshot patches, CRT models/scaling, renderer recreation, density/safe-area code, app identity and Documents path implementations remain unchanged. Shared upstream focus handling retains native cancellation/resume logic. CRT remains session-only and independent of Apply/Reset. No Dynamic Island experiment restored. Compilation and mocked tests do not establish background/resume or GPU runtime acceptance.
- **Branding/reporting:** app display name/icon/bundle identity retained. iOS About/source/report links identify this downstream and credit imbaner; iOS problem reports offer GitHub Issues rather than upstream Form/Telegram destinations. iOS no longer creates a running-marker-based crash inference; normal suspended-session termination is not called a confirmed crash. Original upstream attribution links remain attribution, not downstream report channels.
- **Android updates:** upstream updater implementation retained, including version, package, SHA-256 and signer verification. Downstream Gradle sets `BuildConfig.UPSTREAM_UPDATES=false`; release-package equality alone cannot select upstream Releases. Explicit non-upstream test repositories remain available to test builds. Policy assertions added to the existing Java suite, but not executed locally because Java/Android tools are absent. No downstream release feed, signing change, package-ID change or infrastructure redirection introduced.
- **CI/web:** Balanced workflow unchanged. Web sources retained, without deployment or hosting. Read-only GitHub inspection confirms Auto Format, WebAssembly and Release remain `disabled_manually`; Build remains active. No workflow dispatch or GitHub setting mutation occurred.
- **Scroll limits:** iOS default remains `ignore_scroll_limit=false`, and existing f2_res migration remains applicable. New Settings controls apply/reset/rollback the value; upstream Android/web defaults and migration guards remain platform-specific.

## Automated verification

Environment: macOS arm64 host, Xcode 27.1 (`27A9275`), CMake 4.4.4, pinned SDL2 2.32.8. Host CMake builds universal x86_64/arm64 binaries; executed tests run on the arm64 host. No simulator or device gameplay was run.

Commands below run from the integration worktree. Generated evidence/artifacts remain ignored under `out/` or `/private/tmp/checkpoint-a-*`.

| Command / target | Result and scope |
| --- | --- |
| `cmake -S . -B out/host -DCMAKE_BUILD_TYPE=Debug` | PASS; assertion-enabled host configuration with vendored pinned dependencies. |
| `cmake --build out/host --target regression_tests fallout2-ce ce-dat-tool --parallel 6` | PASS; all host regression executables, desktop engine and resource tool compiled/linked. |
| `cmake --build out/host --target save_catalog_test --parallel 4` | PASS after adding physical relocation-order assertions. |
| `ctest --test-dir out/host --verbose --no-tests=error` | PASS, 18/18. Existing 15 tests retained, plus native touch interruptions, touch view policy, and script serialization. |
| `python3 tests/script_serialization_test.py` | PASS: 2,328 writer/save-parse cases and 608 reader/legacy cases. Also registered in CTest. |
| `python3 tests/touch_view_policy_test.py` | PASS: 16 combinations; also registered in CTest. |
| `mui_main_menu_test`, `mui_main_menu_ios_test` | PASS: each covers 48 menu configurations, input gating, scrolling, Exit behavior and production dispatch. |
| `mui_slider_test`, `mui_slider_ios_test` | PASS: each retains 456 gesture cases and adds independent map/HUD ownership; density/scale, mirrored origins, CRT and segmented bounds checked. |
| `preferences_integration_test`, `game_menu_quick_actions_settings_test` | PASS: production Apply/Reset/rollback, new defaults/persistence, quick actions, CRT independence, unique message IDs, tab/control clipping. Runtime/disk services are mocked. |
| `save_catalog_test` | PASS: real disposable filesystem, identity/chronology/metadata, relocation ordering, interrupted writes/recovery and retained assertions. No actual user saves used. |
| `equipment_drop_control_flow_test` | PASS: 208 production-function checks; inventory actions mocked. |
| `mui_texture_lifetime_test` | PASS including upstream art generation plus downstream F2 upload/lifetime behavior. |
| ASan+UBSan texture harness | PASS. Exact compiler/link command saved in `out/checkpoint-a/sanitizer-command.txt`; binary `out/checkpoint-a/texture-sanitized`. Production `mui_draw.cc`/`mui.cc`, SDL texture storage mocked. |
| `out/host/ce-dat-tool create files/ce.dat out/checkpoint-a/ce.dat` and `test -s out/checkpoint-a/ce.dat` | PASS; nonempty resource archive generated from repository resources. |
| `cmake --preset ios -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO` | PASS; both original SDL patches independently verified. Minimum 15.0 retained. |
| `cmake --build --preset ios-debug --parallel 6` | PASS; final unsigned arm64 iPhoneOS Debug compile/link, SDK 27.1. |
| `cpack --config out/build/ios/CPackConfig.cmake -C Debug -B out/checkpoint-a/package` | PASS; unsigned IPA generated. ZIP/plist inspection confirms original identity/name, minimum 15.0, phone/tablet, both landscapes, File Sharing, no signature/provisioning profile. Mach-O `vtool -show-build` confirms IOS minimum 15.0. |
| `cmake -S . -B out/build/ios-simulator -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO` | PASS; isolated simulator configuration and both SDL patch verifications. |
| `cmake --build out/build/ios-simulator --config Debug --parallel 6` | PASS; unsigned arm64 simulator Debug compile/link, SDK 27.1, IOSSIMULATOR minimum 15.0 verified with `vtool`. No simulator launch. |
| Android APK / Java suite | UNSUPPORTED locally: no Java runtime or Android SDK/NDK found in the configured environment. Source review and updated policy tests are not Android build/runtime evidence. |
| `git diff --check` and Balanced-workflow byte comparison against starting main | PASS. |

Initial failures were resolved fixture/API mismatches: old enum names/signatures, a missing updater stub, attribution links added to the menu inventory, an SVG decimal falsely matching a message ID, and service mocks needed to execute Apply rollback. Valid menu, save, input and settings assertions were retained; new behavior received additional assertions. No integration compiler/test failure remains after the final reruns recorded above.

## Preservation and handoff

Canonical main is not advanced. Historical private checkout/history and all historical worktree registrations are retained, including pre-existing registrations pointing to missing paths. No reset, prune, clean, deletion, remote rewrite or configuration change was performed. Fetching refreshed only canonical source remote-tracking data and normal fetch metadata; the isolated branch/worktree and local merge objects/commit are the authorized additions.

No installed application, Documents directory, actual game/save data, physical device, signing identity, GitHub workflow configuration, release or public ref was modified. No push, PR, release, deployment, signing, installation, launch or public Actions dispatch occurred. No credentials, provisioning files, purchased game archives or generated build artifacts are committed.

Checkpoint B requires separate authorization: simulator gameplay and physical-device acceptance of highlighting/settings; mod-gated appearance and persistence/fallbacks; simultaneous HUD/map input, engine waits and script restrictions; tactical targeting; tab/fade appearance; save/load and quick-save relocation using deliberate test data; safe areas/orientations; CRT and background/resume/renderer recreation; and downstream reporting UI. Compilation and mocked automated tests do not imply that acceptance. Android build/policy tests should run in a supported environment before accepting an Android binary; no downstream Android release channel is introduced here.
