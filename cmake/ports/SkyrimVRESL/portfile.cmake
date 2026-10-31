vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Nightfallstorm/SkyrimVRESL
    REF c261d9ccf313bc23f446f97c254ce992c4d8cb80
    SHA512 3e1396d7a3cbdb00f086d1e600757685d27d10a9a0f27901519fad2b0accdce8208465b39176d6b01c7b31f5e266b4bb53a2f54fb30249f8f34c8b6a066c3530
    HEAD_REF c261d9ccf313bc23f446f97c254ce992c4d8cb80
)

# CommonLib API header (default) — goes to include/ for normal #include <SkyrimVRESLAPI.h>
file(INSTALL "${SOURCE_PATH}/src/SkyrimVRESLAPI.h"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include")

# CommonLib API source — goes to share so the CMake target can reference it via INTERFACE_SOURCES
file(INSTALL "${SOURCE_PATH}/src/SkyrimVRESLAPI.cpp"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/src")

# Legacy SKSE64 SDK variant (optional feature)
if("legacy" IN_LIST FEATURES)
    file(INSTALL "${SOURCE_PATH}/src/SkyrimVRESLAPI_SKSE.h"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include")
    file(INSTALL "${SOURCE_PATH}/src/SkyrimVRESLAPI_SKSE.cpp"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/src")
endif()

# CMake package config — defines SkyrimVRESL::CommonLib and (optionally) SkyrimVRESL::SKSE
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/skyrimvresl-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

file(INSTALL "${SOURCE_PATH}/LICENSE"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
