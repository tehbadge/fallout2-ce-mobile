# SDL 2.32.8 only. This script runs against an isolated iOS build-tree checkout.
# The caller includes the patch hash in SOURCE_DIR so patch revisions start clean.
if(NOT IS_DIRECTORY "${SDL_SOURCE_DIR}" OR NOT EXISTS "${SDL_PATCH}")
    message(FATAL_ERROR "SDL2 iOS patch: SDL_SOURCE_DIR and SDL_PATCH must exist")
endif()
# The caller selects Git for both population and reuse verification. Do not
# rediscover it under Xcode's different PATH, which can change diff hunk headers.
if(NOT IS_ABSOLUTE "${GIT_EXECUTABLE}" OR NOT EXISTS "${GIT_EXECUTABLE}")
    message(FATAL_ERROR "SDL2 iOS patch requires the caller's absolute GIT_EXECUTABLE")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE _revision OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result)
set(_expected "98d1f3a45aae568ccd6ed5fec179330f47d4d356")
if(NOT _result EQUAL 0 OR NOT _revision STREQUAL _expected)
    message(FATAL_ERROR "SDL2 iOS patch: expected SDL 2.32.8 revision ${_expected}, got '${_revision}'. Review the patch before updating SDL.")
endif()

# Both patches and this script participate in the caller's isolated-source hash.
if(NOT EXISTS "${SDL_METAL_PATCH}")
    message(FATAL_ERROR "SDL2 experimental Metal patch must exist")
endif()
set(_patches "${SDL_PATCH}" "${SDL_METAL_PATCH}")
set(_stamp "${SDL_SOURCE_DIR}/.f2ce-patched-diff-sha256")
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
    diff --quiet HEAD RESULT_VARIABLE _dirty)
if(_dirty EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
        apply --check ${_patches} RESULT_VARIABLE _check ERROR_VARIABLE _error)
    if(NOT _check EQUAL 0)
        message(FATAL_ERROR "SDL2 patches do not apply cleanly: ${_error}")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
        apply ${_patches} RESULT_VARIABLE _apply ERROR_VARIABLE _error)
    if(NOT _apply EQUAL 0)
        message(FATAL_ERROR "SDL2 patch application failed: ${_error}")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
        diff --binary --no-ext-diff HEAD OUTPUT_VARIABLE _diff COMMAND_ERROR_IS_FATAL ANY)
    string(SHA256 _hash "${_diff}")
    file(WRITE "${_stamp}" "${_hash}")
else()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
        apply --reverse --check ${_patches} RESULT_VARIABLE _complete OUTPUT_QUIET ERROR_QUIET)
    if(NOT _complete EQUAL 0 OR NOT EXISTS "${_stamp}")
        message(FATAL_ERROR "SDL2 patch mismatch: incomplete or unverified isolated source; use a pristine isolated source")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SDL_SOURCE_DIR}"
        diff --binary --no-ext-diff HEAD OUTPUT_VARIABLE _diff COMMAND_ERROR_IS_FATAL ANY)
    string(SHA256 _hash "${_diff}")
    file(READ "${_stamp}" _expected_hash)
    if(NOT _hash STREQUAL _expected_hash)
        message(FATAL_ERROR "SDL2 patch mismatch: isolated source changed after patch application")
    endif()
endif()
message(STATUS "SDL2 scene-lifecycle and experimental Metal snapshot patches verified")
