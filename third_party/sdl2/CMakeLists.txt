# Build static lib only
set(SDL_SHARED OFF)
set(SDL_STATIC ON)

# Fixes _ftol2_sse already defined
set(SDL_LIBC ON)

if (ANDROID)
    set(SDL_STATIC_PIC ON)
endif()

include(FetchContent)

set(_sdl2_ios_options)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Never patch a shared dependency checkout used by Android or desktop builds.
    if(FETCHCONTENT_SOURCE_DIR_SDL2)
        message(FATAL_ERROR "The iOS SDL2 patch requires its isolated build-directory source; unset FETCHCONTENT_SOURCE_DIR_SDL2")
    endif()
    # Xcode's population build can have a different PATH from configure.
    # Select Git once: textual diff fingerprints must use the same executable.
    find_package(Git REQUIRED)
    get_filename_component(_sdl2_ios_git "${GIT_EXECUTABLE}" REALPATH)
    file(SHA256 "${_sdl2_ios_git}" _git_hash)
    set(_sdl2_ios_patch "${CMAKE_CURRENT_LIST_DIR}/patches/ios-scene-lifecycle.patch")
    set(_sdl2_ios_script "${CMAKE_SOURCE_DIR}/cmake/ApplySdl2IosPatch.cmake")
    set(_sdl2_metal_patch "${CMAKE_CURRENT_LIST_DIR}/patches/ios-metal-snapshot.patch")
    file(SHA256 "${_sdl2_ios_patch}" _lifecycle_hash)
    file(SHA256 "${_sdl2_metal_patch}" _metal_hash)
    file(SHA256 "${_sdl2_ios_script}" _script_hash)
    string(SHA256 _sdl2_ios_patch_hash "${_lifecycle_hash};${_metal_hash};${_script_hash};${_sdl2_ios_git};${_git_hash}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${_sdl2_ios_patch}" "${_sdl2_metal_patch}" "${_sdl2_ios_script}")
    # Changed patches, verifier logic or Git identity get pristine isolated source.
    list(APPEND _sdl2_ios_options
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/sdl2-ios-${_sdl2_ios_patch_hash}-src"
        PATCH_COMMAND "${CMAKE_COMMAND}"
            "-DGIT_EXECUTABLE=${_sdl2_ios_git}"
            "-DSDL_SOURCE_DIR=<SOURCE_DIR>"
            "-DSDL_PATCH=${_sdl2_ios_patch}" "-DSDL_METAL_PATCH=${_sdl2_metal_patch}"
            -P "${_sdl2_ios_script}")
endif()

FetchContent_Declare(sdl2
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL"
    GIT_TAG "release-2.32.8"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    EXCLUDE_FROM_ALL
    ${_sdl2_ios_options}
)

FetchContent_MakeAvailable(sdl2)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Also validate reused population stamps on every configure.
    execute_process(COMMAND "${CMAKE_COMMAND}"
        "-DGIT_EXECUTABLE=${_sdl2_ios_git}"
        "-DSDL_SOURCE_DIR=${sdl2_SOURCE_DIR}" "-DSDL_PATCH=${_sdl2_ios_patch}" "-DSDL_METAL_PATCH=${_sdl2_metal_patch}"
        -P "${_sdl2_ios_script}" COMMAND_ERROR_IS_FATAL ANY)
endif()

set(SDL2_INCLUDE_DIRS ${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR} PARENT_SCOPE)
set(SDL2_LIBRARIES SDL2-static PARENT_SCOPE)
set(SDL2_MAIN_LIBRARIES SDL2::SDL2main PARENT_SCOPE)
