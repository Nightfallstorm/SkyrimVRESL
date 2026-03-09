vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Nightfallstorm/SkyrimVRESL
    REF 128a8399a7c3dc92206426a69fb6bbd32af2e1d1
    SHA512 de91249453579685065ec8f7586a72ee353471a387482b9fab17d557c50233f22a2732022e3e846876f7dea8e13ef5e5050c00b4692088d428c8833c6bdf0769
    HEAD_REF main
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
