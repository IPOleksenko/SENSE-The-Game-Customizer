cmake_minimum_required(VERSION 3.21)
project(SENSE_THE_GAME_CUSTOMIZER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(ASSETS_DIR ${PROJECT_ROOT}/assets)
set(SOURCE_DIR ${PROJECT_ROOT}/cpp)

include(sdl2_import.cmake)
include(incbin_import.cmake)
include(imgui_import.cmake)
if(NOT ANDROID)
    include(steamSDK_import.cmake)
    include(tinyfiledialogs_import.cmake)
endif()

include(${SOURCE_DIR}/assets/module.cmake)
include(${SOURCE_DIR}/utils/module.cmake)
include(${SOURCE_DIR}/objects/module.cmake)
include(${SOURCE_DIR}/application/module.cmake)

set(SRC_FILES
    ${SOURCE_DIR}/main.cpp
)

if(ANDROID)
    include(android_config.cmake)

    add_library(${PROJECT_NAME} SHARED
        ${SRC_FILES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${SRC_FILES}
    )

    install(
        TARGETS
            ${PROJECT_NAME}
            SDL2
            SDL2_image
            SDL2_ttf
            imgui
            imgui-sdl2
            imgui-sdlrenderer2
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2main
        imgui
        imgui-sdl2
        imgui-sdlrenderer2
        ${PROJECT_NAME}_application
)

if (NOT ANDROID)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            steam_api
            tinyfiledialogs
    )
endif()

if(MSVC)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources.rc
        @ONLY
    )
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON_RESOURCE})
    
    include(msvc_config.cmake)
endif()

if(UNIX)
    include(install_launcher.cmake)
endif()

if(NOT ANDROID)
    if(NOT DEFINED STEAM_SDK_DIR)
        set(STEAM_SDK_DIR "${PROJECT_ROOT}/thirdparty/steamworks_sdk")
    endif()

    if(WIN32)
        # Windows
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(STEAM_ARCH "win64")
            set(STEAM_API_LIB "${STEAM_SDK_DIR}/redistributable_bin/${STEAM_ARCH}/steam_api64.dll")
        else()
            set(STEAM_ARCH "win32")
            set(STEAM_API_LIB "${STEAM_SDK_DIR}/redistributable_bin/steam_api.dll")
        endif()
    elseif(APPLE)
        # macOS
        set(STEAM_ARCH "osx")
        set(STEAM_API_LIB "${STEAM_SDK_DIR}/redistributable_bin/${STEAM_ARCH}/libsteam_api.dylib")
    elseif(UNIX)
        # Linux
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(STEAM_ARCH "linux64")
        else()
            set(STEAM_ARCH "linux32")
        endif()
        set(STEAM_API_LIB "${STEAM_SDK_DIR}/redistributable_bin/${STEAM_ARCH}/libsteam_api.so")
    endif()

    # Verify existence
    if(NOT EXISTS "${STEAM_API_LIB}")
        message(WARNING "Steam API binary not found for ${STEAM_ARCH} at ${STEAM_API_LIB}")
    else()
        message(STATUS "Using Steam API: ${STEAM_API_LIB}")

        # Copy to build directory
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${STEAM_API_LIB}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )

        install(FILES "${STEAM_API_LIB}"
                DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
        )
    endif()
endif()