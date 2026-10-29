# Settings slider gesture correction — physically accepted (2026-10-20)

Historical private-development report: downstream commit references and local evidence below are preserved as technical records, not commits or artifacts available in the consolidated public history. See [current status](STATUS.md#current-accepted-state-2026-10-28).

Milestone 1C is physically accepted and committed as `c909060aa4fb64954d7e17e9e7dd9132940037c7` (`ios: fix constrained main menu layout`). The physically accepted slider correction is preserved by the local commit `fix: prevent slider changes during settings scrolling`. Offline checkpoint results below are retained; subsequent physical deployment and user acceptance are recorded at the end. No push, fetch, merge, or branch/remote change occurred.

## Independence and cause

Before the 1C commit, only `src/mui_main_menu.cc` and `docs/ios-port/STATUS.md` differed from parent `96835b1b0833047dcfd23ec78b9d55478e33acef`. `git diff HEAD -- src/mui.cc src/mui_preferences.cc` was empty. The parent `MuiContext::slider` called `touchable`, then immediately mapped the pointer x coordinate to a value whenever the finger was down. Thus the defect existed in unaffected shared code, independently of menu layout.

Settings calls `ui.scroll("prefs.rows", ...)` before building its child rows. A child slider claims `prefs.rows.<id>` on press. The container can replace that child owner with `prefs.rows` once vertical movement exceeds `dp(theme.touchSlop)` (10 dp) and dominates horizontal movement. Previously, that recognition came after the slider had already changed on the press frame. `PreferencesScreen::buildRow` immediately retains the returned value in `edited`; volume and brightness also preview immediately. Disk persistence occurs through the existing Apply/confirmed Back path, rather than on each pointer event.

Android uses the same C++ handlers and ordering. The parent behavior is not iOS-specific.

## Source changes

- `src/mui.cc`: per-touch pending/horizontal/vertical slider decision, reset for a new touch/owner. Press alone only highlights. A stationary release commits a tap; horizontal movement beyond the existing scaled slop starts adjustment in the same frame. Values remain clamped and the release coordinate completes a horizontal drag. A recognized horizontal adjustment prevents the scroll container from stealing it later. Vertical gestures never speculatively update or revert a setting; the existing container capture cancels the child. At exact diagonal equality, adjustment is suppressed; scrolling starts if vertical movement subsequently dominates.
- `src/mui.h`: document deferred updates and the adjustment flag, including the tap-release frame. This preserves Settings' existing volume-sample completion mechanism.
- `tests/mui_slider_test.cc`: execute the production pointer queue, slider, and scroll methods with drawing stubs, including the non-scrolling quantity-dialog case. No separate gesture implementation is tested.
- `CMakeLists.txt`: register the host test, retain assertions, and discard unrelated engine entry points during linking.
- `docs/ios-port/STATUS.md` and this report: current results and validation limitations.

No Settings row geometry, scale/density policy, safe-area conversion, gameplay input, HUD, storage code, SDL2 patch, or lifecycle code changed. The quantity dialog also uses this shared slider, so its tap now commits on release and its horizontal drag uses the same slop.

## Validation

| Check | Result |
| --- | --- |
| iPhoneOS arm64 Debug, minimum iOS 15.0, unsigned | Build passed |
| iPhoneSimulator arm64 Debug, minimum iOS 15.0, unsigned | Build passed |
| macOS arm64 Debug game and mapper | Build passed; desktop runtime not exercised |
| Registered host slider test | 456 gesture cases passed, assertions enabled |
| Existing display-geometry host test | Passed |
| `git diff --check` | Passed |
| Android | SDK/NDK unavailable; no build or runtime test |

Host cases cover tap/release, subthreshold movement, horizontal and vertical drags, scrolling outside a slider, both diagonal directions/equality, lost ownership, disabled interaction, quick queued taps, 20 rapid repeated gestures, release without a move event, min/max clamping, zero range, and non-scrolling sliders. Each runs at densities 1/2/3 and scales 100/125/150/200%, with two shifted landscape origins. These origin tests do not establish UIKit runtime rotation.

Actual shared Settings ran on the iPhone 18 Pro simulator, iOS 27.0, 874×402 points, density 3. Synthetic SDL gestures were injected through the existing developer scenario; LLDB recorded `edited.preferences.combatSpeed` and the actual `prefs.rows` offset each frame. Scale and movie-skip fixtures were in process memory. No save/load operations ran.

| UI scale | Largest offset during vertical-slider step (output pixels) | Combat speed throughout that step | Subsequent horizontal drag / center tap |
| --- | --- | --- | --- |
| 100% | 254.6 | 0, unchanged | Reached 44 / 25 |
| 125% | 253.4 | 0, unchanged | Reached 50 / 25 |
| 150% | 363.1 | 0, unchanged | Reached 50 / 25 |

Each run restored the list to its top, verified adjustment with no scroll displacement, switched away from and back to Combat with the edited value retained, and discarded changes through Back. A final run against the final simulator build repeated the 150% gestures, applied value 25, and loaded 25 in a fresh process. The test then restored the original configuration state: this simulator had no `fallout2.cfg`, so only the newly generated test configuration was removed. No game asset or save was edited. Settings screenshots before/after scrolling were visually inspected.

The pre-menu fixture attempt ran during startup movies and failed to find menu widgets. It is excluded from passing results; later fixtures skip movies in memory and finish successfully. Existing developer logs print placeholder world-view/dude-tile diagnostics without a loaded map; those are not gameplay checks.

Ignored evidence: `out/slider-evidence/` contains build/test logs, LLDB fixtures and frame traces, persistence result, and the test runner. Screenshots/logs are in `/tmp/slider100`, `/tmp/slider125`, `/tmp/slider150b`, `/tmp/slider-persist`, and `/tmp/slider-relaunch`. Generated evidence and configuration are not included in Git.

## Limits and original physical retest plan

- At the simulator checkpoint, real touch latency, native interruption/background cancellation, and rotation through both landscape orientations remained unverified. Subsequent user-reported physical results appear below; native cancellation remains a limitation. Simulator is unavailable through the enabled native UI controls. Host origin/geometry tests are not substitutes for runtime rotation.
- Direction locks after recognition: a horizontal gesture that later turns vertically continues adjusting and cannot become scrolling. A vertical-first gesture cannot later become an adjustment. Subthreshold jitter waits for release or a decisive movement; no timer was added.
- A full vertical move/release coalesced into one UI update still suppresses the slider change, but the existing scroll recognizer requires a down frame and may not scroll that extremely short gesture. Scroll recognition timing was left intact.
- Lost-ownership/disabled pending gestures are covered. SDL2 UIKit implements `touchesCancelled` by calling `touchesEnded`, producing ordinary finger-up. The shared UI cannot distinguish that native cancellation from a tap release. This existing backend limitation was not changed; native cancellation is not claimed to pass.
- Android's shared slider/quantity behavior changes as described, with no Android-specific geometry or JNI changes. Host tests cover density scaling; Android compilation and actual touch still need validation.

The original physical retest plan, subsequently accepted by user report below, requested a separately authorized in-place device update preserving the existing bundle ID and Documents, followed by tests on iPhone 17 Pro Max at 100/125/150% in **both** landscape orientations:

1. Record Combat speed, volume, brightness, and Quick saves. Start vertical and vertical-diagonal swipes directly over each visible slider, including near both ends. Confirm scrolling works and every original value remains unchanged; Back should not ask to apply changes from scrolling alone.
2. Repeat scrolling outside sliders, small initial movement, exact/near-diagonal movement, quick taps, and rapid alternating gestures. Check no coordinate offset and no unexpected samples or previews on scroll-only gestures.
3. Tap and drag sliders horizontally at their middle and endpoints. Check immediate response after movement slop, smooth adjustment, endpoint clamping, and intended volume/brightness previews. A horizontal drag that turns vertically should stay an adjustment.
4. Interrupt pending touches by leaving the screen/backgrounding; check resume and record native cancellation behavior explicitly. Check the shared quantity dialog's tap and drag.
5. Apply one intentional, reversible adjustment; reopen Settings and restart the app to verify persistence, then restore the original value. Repeat a short main-menu/gameplay/background-resume regression with existing saves.

Physical results were subsequently accepted; the user authorized the local acceptance commit on 2026-10-20.

## Physical deployment and test session (2026-10-20)

- Preflight: branch `ios-port`, HEAD `c909060aa4fb64954d7e17e9e7dd9132940037c7`; pending changes match the source/test/documentation list above. No unexpected changes or significant defect identified in diff review. `git diff --check` passed. No additional source changes, commits, branch/remote changes, or upstream operations.
- Offline configuration verified the existing SDL2 lifecycle patch already applied. Fresh iPhoneOS arm64 Debug Personal Team signing/build passed; minimum iOS 15.0. Signed plist preserves the complete generated scene manifest and both landscape declarations; bundle ID remains `[preserved development bundle ID]`. Strict signature verification passed.
- Connected physical iPhone 17 Pro Max; installed in place and launched successfully. Expected display metrics logged: 956×440 points, 3× density, orientation 3, safe L/T/R/B 62/0/62/20. This establishes launch, not physical gesture acceptance.
- Fresh complete Documents backup: `~/Fallout2-iOS-Backups/slider-20261020/Documents`, 169 regular files / 582,649,815 bytes, including 134 save-related files. Inventory sizes match, backup SHA-256 manifest verified, and device inventory stable during copy. No old backup replaced. All 169 file sizes/mtimes identical after installation before launch. All 134 save-related files read back after launch match the pre-install hashes.
- Saved baseline before tests: combat speed 0; text delay 3.5 s; brightness 1.0 (displayed 0%); master/music/effects/speech volumes each 22281 (68%); Quick saves Off; interface size 100%. The user subsequently reported all testing passed; displayed values were not individually transcribed in the reply.
- Local ignored evidence: `out/slider-device-*`, `out/slider-documents-*`, `out/slider-backup-*`, `out/slider-save-readback*`, `out/slider-physical-configure.log`.

| Scale | Dynamic Island side | Vertical swipes / result | Horizontal / tap / ambiguity | Physical result |
| --- | --- | --- | --- | --- |
| 100% | Left | Pass; count not separately reported | Pass by user report | Accepted by user |
| 100% | Right | Pass; count not separately reported | Pass by user report | Accepted by user |
| 125% | Left | Pass; count not separately reported | Pass by user report | Accepted by user |
| 125% | Right | Pass; count not separately reported | Pass by user report | Accepted by user |
| 150% | Left | Pass; count not separately reported | Pass by user report | Accepted by user |
| 150% | Right | Pass; count not separately reported | Pass by user report | Accepted by user |

User confirmation: “all tests passed, change successful. If I try to swipe vertically on a horizontal slider, the slider button lights up but does not move. All testing complete, dynamic island on the left or right, all ui scales.” Physical acceptance is therefore confirmed for all six configurations. Highlighting without adjustment is expected pressed feedback, not an unintended setting change.

The user’s blanket all-tests-passed confirmation covers the requested tap/horizontal/endpoints/ambiguity checks, interruption where practical, intentional-change and scroll-only persistence, restoration of original settings, ordinary scrolling/Back, existing-save loading, movement/camera/HUD regression, and continued data/save access. These are user-reported results, not independently observed per-step traces. Exact swipe counts were not supplied (the requested minimum was 10 per configuration), and no individual interruption sequence was described. No ambiguous/interrupted failure was reported; SDL native cancellation still maps to finger-up and this session does not prove every cancellation path safe. No new defect reported. Android runtime remains untested. The physical slider correction is accepted. The subsequent acceptance task authorizes one local commit of the source, tests, and documentation, with no additional testing, deployment, or milestone work.
