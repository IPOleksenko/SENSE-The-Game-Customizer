include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    SOURCE_DIR ${PROJECT_ROOT}/external/imgui
)

FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

if(ANDROID)
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(imgui ${_lib_type}
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

add_library(imgui-sdl2 ${_lib_type}
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
)
target_include_directories(imgui-sdl2 PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui-sdl2 PUBLIC imgui SDL2::SDL2 SDL2::SDL2main)

add_library(imgui-sdlrenderer2 ${_lib_type}
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_include_directories(imgui-sdlrenderer2 PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui-sdlrenderer2 PUBLIC imgui SDL2::SDL2)
