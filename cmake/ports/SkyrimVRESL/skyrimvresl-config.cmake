get_filename_component(_SKYRIMVRESL_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# CommonLib-based API target (default)
# Consuming target gets the include dir and SkyrimVRESLAPI.cpp compiled into it.
if(NOT TARGET SkyrimVRESL::CommonLib)
    add_library(SkyrimVRESL::CommonLib INTERFACE IMPORTED)
    set_target_properties(SkyrimVRESL::CommonLib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_SKYRIMVRESL_DIR}/../../include"
        INTERFACE_SOURCES             "${_SKYRIMVRESL_DIR}/src/SkyrimVRESLAPI.cpp"
    )
endif()

# Legacy SKSE64 SDK target (optional, installed via the "skse" feature)
if(EXISTS "${_SKYRIMVRESL_DIR}/src/SkyrimVRESLAPI_SKSE.cpp")
    if(NOT TARGET SkyrimVRESL::SKSE)
        add_library(SkyrimVRESL::SKSE INTERFACE IMPORTED)
        set_target_properties(SkyrimVRESL::SKSE PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_SKYRIMVRESL_DIR}/../../include"
            INTERFACE_SOURCES             "${_SKYRIMVRESL_DIR}/src/SkyrimVRESLAPI_SKSE.cpp"
        )
    endif()
endif()
