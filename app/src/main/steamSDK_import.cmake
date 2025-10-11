set(STEAM_SDK_DIR ${PROJECT_ROOT}/sdk/steamworks_sdk_162)

if(NOT EXISTS "${STEAM_SDK_DIR}/public/steam/steam_api.h")
    message(FATAL_ERROR "Steamworks SDK not found in ${STEAM_SDK_DIR}")
endif()

add_library(steam_api INTERFACE)
target_include_directories(steam_api INTERFACE
    ${STEAM_SDK_DIR}/public
    ${STEAM_SDK_DIR}/public/steam
)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(STEAM_LIB_DIR "${STEAM_SDK_DIR}/redistributable_bin/win64")
        set(STEAM_LIB "${STEAM_LIB_DIR}/steam_api64.lib")
        set(STEAM_DLL "${STEAM_LIB_DIR}/steam_api64.dll")
    else()
        set(STEAM_LIB_DIR "${STEAM_SDK_DIR}/redistributable_bin")
        set(STEAM_LIB "${STEAM_LIB_DIR}/steam_api.lib")
        set(STEAM_DLL "${STEAM_LIB_DIR}/steam_api.dll")
    endif()
elseif(APPLE)
    set(STEAM_LIB_DIR "${STEAM_SDK_DIR}/redistributable_bin/osx")
    set(STEAM_LIB "${STEAM_LIB_DIR}/libsteam_api.dylib")
elseif(UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(STEAM_LIB_DIR "${STEAM_SDK_DIR}/redistributable_bin/linux64")
    else()
        set(STEAM_LIB_DIR "${STEAM_SDK_DIR}/redistributable_bin/linux32")
    endif()
    set(STEAM_LIB "${STEAM_LIB_DIR}/libsteam_api.so")
endif()

add_library(steam_api_lib STATIC IMPORTED)
set_target_properties(steam_api_lib PROPERTIES
    IMPORTED_LOCATION "${STEAM_LIB}"
)

target_link_libraries(steam_api INTERFACE steam_api_lib)
