# Contributing

This downstream preserves Community Engine and imbaner’s shared mobile port. Keep platform changes focused, retain Android/desktop compatibility and upstream attribution, and follow [AGENTS.md](AGENTS.md) for data-preservation and publication safeguards. The published downstream source is [tehbadge/fallout2-ce-mobile](https://github.com/tehbadge/fallout2-ce-mobile). Use [downstream Issues](https://github.com/tehbadge/fallout2-ce-mobile/issues) for native iOS, downstream mobile UI, additional fixes/CRT, and source build/run questions. Original Android-release issues that reproduce on unchanged upstream should generally go to [imbaner’s tracker](https://github.com/imbaner/fallout2-ce-mobile/issues). The public Balanced Build workflow is active and has passed its first public validation; see [Public CI validation](#public-ci-validation). Auto Format and Release remain disabled. No installable iOS binary release has been approved.

For current sfall compatibility status and the remaining work needed to close gaps, see [SFALL_COMPATIBILITY.md](SFALL_COMPATIBILITY.md).

## Building

The project uses CMake and C++17. The checked-in presets use schema version 6 and need **CMake 3.25 or later** (the older project minimum does not describe preset compatibility). Native builds fetch pinned dependencies by default; Git and network access are needed for a fresh dependency checkout. Never commit build trees, game files or signing material.

### iOS / iPadOS

Use macOS with **full Xcode**, its iPhoneOS SDK and command-line tools selected, Git, and CMake 3.25+. Command Line Tools alone do not provide the iPhoneOS SDK. Historical local validation used Xcode 27.1/CMake 4.4.4; the accepted hosted IPA records Xcode 15.4/SDK 17.5. These are observed build environments, not a guaranteed minimum Xcode/CMake compatibility matrix; 3.25 is the preset-parser requirement, and lower complete-build tool combinations have not been established here.

From the repository root, the existing presets produce an unsigned device build:

```sh
cmake --preset ios -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build --preset ios-debug
```

The preset uses the Xcode generator and `out/build/ios`. The deployment policy enforces **iOS/iPadOS 15.0 minimum** and defaults to arm64. `ios-release` selects **RelWithDebInfo**, despite its name. A simulator requires a separate build directory with `CMAKE_OSX_SYSROOT=iphonesimulator`; it is not device-installation evidence.

The iOS configuration fetches SDL **2.32.8** into an isolated build-directory checkout and applies/verifies both `ios-scene-lifecycle.patch` and `ios-metal-snapshot.patch`. Do not patch a shared dependency checkout or override `FETCHCONTENT_SOURCE_DIR_SDL2`; verification deliberately rejects that override. Changing or bypassing patch verification is not part of a normal build.

For optional unsigned packaging after the Debug build:

```sh
(cd out/build/ios && cpack -C Debug)
```

This creates `out/build/ios/fallout2-ce.ipa`. It does not sign the application, generate `ce.dat`, install it, or validate gameplay. Public CI runs CPack only for validation and uploads no IPA, application binaries or resource archives. The standalone IPA contains neither `ce.dat` nor `EXAMPLE_fallout2.cfg`; supply matching resources separately.

#### Matching resources

Build `ce.dat` from **the same source revision** as the engine. The iOS target cannot build the host `ce-dat-tool`, so use a separate native macOS build. For example, with Ninja installed:

```sh
cmake -S . -B out/build/ce-resources -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES="$(uname -m)"
cmake --build out/build/ce-resources --target ce-dat-tool
mkdir -p out/dist
out/build/ce-resources/ce-dat-tool create files/ce.dat out/dist/ce.dat
```

The tool’s archive syntax and targets are source-verified; this exact convenience recipe was not newly executed for the documentation change. The active [Build workflow](.github/workflows/ci-build.yml) uses the same `create files/ce.dat` operation locally in its Linux host job; the older reusable resource workflow remains unchanged and uncalled by Build. A historical private CI ZIP is another resource source only for maintainers with access to that private artifact; public CI uploads no resources. The private development repository is not a public download destination. Do not substitute an older CE release’s `ce.dat` for this mobile UI’s resources. `files/f2_res.dat` is optional; public CI does not distribute resources.

#### Signing, installation and data

A device installation requires Apple signing/provisioning for your account and device. Reconfigure your local device build to allow signing before opening the generated project:

```sh
cmake --preset ios -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=YES
open out/build/ios/fallout2-ce.xcodeproj
```

Choose your development team and device in Xcode and resolve its signing/trust requirements. Keep overrides local. The preceding unsigned compile does not establish that your signing setup works. Development-signed Xcode installation was historically verified, but a fresh-user end-to-end signing/install recipe and third-party sideloading tools are not accepted distribution procedures here. Provisioning renewal depends on the account; this project supplies no credentials or profiles.

Before updating an existing installation, back up Documents and identify its actual bundle ID/signing identity. Preserve that identity and install in place; do not uninstall to fix signing. The source default `com.alexbatalov.fallout2-ce` is not necessarily the identity of an existing development installation. If a local signing override changes the generated plist, derive it from the freshly generated file and preserve the complete SDL scene manifest, device families and both landscape declarations. Do not edit the tracked plist or copy a stale generated one.

The app enables File Sharing and reads files from Documents. Supply your legally acquired Fallout 2 installation archives, relevant `data`/`sound` folders and matching `ce.dat` at the document root. Finder’s device Files tab is the documented transfer mechanism; historical development transfers verified the Documents path and retained saves, but not every Finder/Windows-client folder-transfer workflow. No Android-style folder/archive import UI exists on iOS. Preserve `SAVEGAME` and existing `fallout2.cfg`; use the example config as reference. See [README.md](README.md#install-on-ios--ipados) for controls and acceptance limits.

### Host regression tests

The existing regression suite runs on native POSIX desktop hosts, not iOS. For the CI dependency mode on Ubuntu, install CMake, a C++ compiler, SDL2/zlib development packages and Python 3, then:

```sh
cmake -S . -B out/regression -DCMAKE_BUILD_TYPE=Debug -DFALLOUT_VENDORED=OFF
cmake --build out/regression --target regression_tests --parallel 2
ctest --test-dir out/regression --verbose --no-tests=error
```

On macOS, use matching native-architecture SDL2/zlib development packages and Python 3; select `CMAKE_OSX_ARCHITECTURES` to match installed libraries if needed. Test targets keep assertions enabled. At historical private implementation baseline `e4cc4f1a` (preserved in the consolidated source snapshot), all **15** tests passed in the hosted Ubuntu 22.04 job. They need no purchased game archives, device or signing credentials. The iOS-guarded harnesses are host tests, not UIKit/Metal/device runtime acceptance. See [Build workflow](.github/workflows/ci-build.yml) and [current iOS status](docs/ios-port/STATUS.md#current-accepted-state-2026-10-28).

## Public CI validation

The active [Build workflow](.github/workflows/ci-build.yml) has three independent jobs:

- **Host regressions and Linux engine:** Ubuntu 22.04, assertion-enabled Debug tests using the [host regression commands](#host-regression-tests), full `fallout2-ce` executable compilation, and local `ce.dat` generation. Reuse the same configuration after the regression commands:

  ```sh
  cmake --build out/regression --target fallout2-ce ce-dat-tool --parallel 2
  mkdir -p out/dist
  out/regression/ce-dat-tool create files/ce.dat out/dist/ce.dat
  test -s out/dist/ce.dat
  ```

- **iOS:** fresh dependencies with both SDL patches, unsigned iPhoneOS arm64 Debug compilation and CPack using the [existing presets and commands](#ios--ipados).
- **Android:** the existing JDK 17/Gradle configuration, running `cd os/android` then `./gradlew assembleDebug` with all configured ABIs (`armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`), without signing secrets.

Build runs for pushes to `main`, pull requests targeting `main` (opened, synchronize and reopened), and explicit manual dispatch. Push/PR changes consisting only of Markdown files (`**/*.md`) are skipped; mixed source/documentation changes and workflow-file changes remain eligible. Manual dispatch has no path filter.

The workflow uses read-only contents permission and uploads no build artifacts, application binaries or resource archives. Debug APK and unsigned IPA creation are temporary runner-side build operations. Compilation, regression and packaging checks do not establish signing, installation, gameplay, rendering or physical-device compatibility. Record device testing separately. The [first public validation](https://github.com/tehbadge/fallout2-ce-mobile/actions/runs/40819456524) passed all three jobs; see [current iOS status](docs/ios-port/STATUS.md#balanced-public-ci-activated-and-accepted-2026-10-28) for the milestone evidence. Historical private CI evidence above retains its separate scope.

## Commandline arguments

- `--scan-unimplemented` - do an analysis of opcodes and hooks used by loaded mods
- `--dev-load-game=1` - load the given save game automatically (useful for LLM automation)
- `--dev-endgame` - after a new or loaded game starts, trigger the normal `endgame_slideshow` request
- `--dev-endgame-movie` - after a new or loaded game starts, run the `endgame_movie` sequence directly

For Fallout 1/Fo1in2-style endings, use both endgame flags to mirror the script flow:

```sh
./Fallout\ II\ Community\ Edition.app/Contents/MacOS/Fallout\ II\ Community\ Edition --dev-load-game=1 --dev-endgame --dev-endgame-movie
```

### macOS

- Install the Command Line Tools if needed: `xcode-select --install`
- Install the common build dependencies: `brew install cmake ninja`
- Configure and build:

```sh
cmake -S . -B out/build/macos -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/macos --target fallout2-ce
```

### Linux

Install SDL and a toolchain:

- Debian / Ubuntu:
```sh
sudo apt install build-essential cmake ninja-build libsdl2-dev zlib1g-dev
```
- Fedora:
```sh
sudo dnf install gcc-c++ cmake ninja-build SDL2-devel zlib-devel
```
- Arch Linux:
```sh
sudo pacman -S base-devel cmake ninja sdl2 zlib
```

Then build:
```
cmake -S . -B out/build/linux-x64-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/linux-x64-debug --target fallout2-ce
```

### Windows

TODO: add Visual Studio build instructions. This will likely need a Visual Studio generator or preset plus the MSVC toolchain and Windows SDK.

## `.dat` CLI Tool

There is a small command-line archive tool in this repo for inspecting Fallout `.dat` files. It supports both Fallout 2 and Fallout 1 archives.

From the repo root, build it with your normal CMake workflow, targeting `ce-dat-tool`:

`cmake --build out/build/<preset-name> --target ce-dat-tool`

The executable is written to your selected build directory, so the exact path will vary by preset and configuration.

Available commands:

1. `./<BUILD_DIR>/ce-dat-tool <archive.dat> list [pattern]`
2. `ce-dat-tool <archive.dat> info [pattern]`
3. `ce-dat-tool <archive.dat> extract [--lower] <output-dir> [pattern]`
4. `ce-dat-tool <archive.dat> extract [--lower] (--file-list|--files-from) <list-file> <output-dir>`
5. `ce-dat-tool <archive.dat> cat <entry>`
6. `ce-dat-tool create <input-dir> <archive.dat>`

Use `--lower` with `extract` when you want every extracted file and directory name forced to lowercase.
For example:

`ce-dat-tool master.dat extract --lower /tmp/ce-dat-tool-lower data\\*`

## `ce-frm2png` CLI Tool

There is also a small FRM conversion tool in this repo for converting Fallout art between `.frm` and `.png`.

From the repo root, build it with your normal CMake workflow, targeting `ce-frm2png`:

`cmake --build out/build/<preset-name> --target ce-frm2png`

The executable is written to your selected build directory, so the exact path will vary by preset and configuration.

Basic usage:

1. `./<BUILD_DIR>/ce-frm2png <input.frm> [output.png]`
2. `./<BUILD_DIR>/ce-frm2png <input.frm> [output.png] --palette <path-to-color.pal>`
3. `./<BUILD_DIR>/ce-frm2png <input.frm> [output.png] --frame <index> --direction <index>`
4. `./<BUILD_DIR>/ce-frm2png <input.png> [output.frm] --palette <path-to-color.pal>`
5. `./<BUILD_DIR>/ce-frm2png <input.png> [output.frm] --fps 10 --action-frame 0 --x-offset 0 --y-offset 0`
6. `./<BUILD_DIR>/ce-frm2png <input.png> <output.png> --palette <path-to-color.pal>`
7. `./<BUILD_DIR>/ce-frm2png - [output.png|-] --from-frm --palette <path-to-color.pal>`
8. `./<BUILD_DIR>/ce-frm2png - [output.frm|-] --from-png --palette <path-to-color.pal>`

`-` can be used to take input from stdin (e.g. from `ce-dat-tool cat`) or write to stdout. When reading from stdin, pass `--from-frm` or `--from-png`. Output defaults to PNG for FRM input and FRM for PNG input when output is omitted or `-`; use `--to-png` for PNG output to stdout from PNG input.

Output file extensions are validated against the selected conversion. For example, PNG-to-FRM output must use `.frm`, and PNG output must use `.png`.

If `--palette` isn't specified, it will pick up `color.pal` in the input file's directory or cwd.

Notes:

- The tool expects a raw `color.pal` file. It does not read palettes directly from `master.dat`, so extract `color.pal` first if needed.
- A typical extraction flow is: `ce-dat-tool master.dat extract --lower . 'color.pal'`
- By default palette index `0` becomes transparent in the output PNG. Pass `--opaque` to keep it opaque.
- PNG output is always palette-indexed and preserves Fallout palette indices when possible.
- For `png -> frm`, transparent pixels (alpha below 128) map to palette index `0` by default. Pass `--opaque` to quantize every pixel instead.
- The current `png -> frm` implementation writes a single-frame FRM and points all six direction slots at the same frame payload.

## Updating SDL

SDL is pinned for native builds in `third_party/sdl2/CMakeLists.txt`; iOS also verifies the two patches described above, which must be reviewed against any new SDL version. Right now, Android also relies on checked-in Java bindings in `os/android/app/src/main/java/org/libsdl/app`, and those bindings must match the SDL version fetched by CMake.

When updating SDL:

1. Update the SDL tag in `third_party/sdl2/CMakeLists.txt`.
2. Run an Android build once so Gradle/CMake fetch the new SDL source tree.
3. Copy the Android Java bindings from the fetched SDL checkout into `os/android/app/src/main/java/org/libsdl/app`.
   Source path pattern:
   `os/android/app/.cxx/<Variant>/<Hash>/arm64-v8a/_deps/sdl2-src/android-project/app/src/main/java/org/libsdl/app`
4. Rebuild Android and confirm the SDL sync guard passes.

The Android build contains a guard in `os/android/app/build.gradle` that fails if the checked-in Java bindings drift from the fetched native SDL source.

This is a hack, not a good long-term setup. The Java bindings are still duplicated in the repo instead of being sourced from the same SDL tree as the native build. That should be improved so Android does not depend on a manual copy/sync step.
