set(MODULE_NAME objects)
set(MODULE_DIR ${SOURCE_DIR}/${MODULE_NAME})
set(INCLUDE_DIR ${MODULE_DIR}/${MODULE_NAME})
set(MODULE_TARGET ${PROJECT_NAME}_${MODULE_NAME})

set(MODULE_SOURCES
    ${MODULE_DIR}/text.cpp
    ${MODULE_DIR}/imgui_window.cpp
)

set(MODULE_HEADERS
    ${INCLUDE_DIR}/text.hpp
    ${INCLUDE_DIR}/imgui_window.hpp
)

add_library(
    ${MODULE_TARGET} STATIC
        ${MODULE_SOURCES}
        ${MODULE_HEADERS}
)

target_include_directories(
    ${MODULE_TARGET} PUBLIC
        ${MODULE_DIR}
        ${UTILS_MODULE_DIR}
)

target_link_libraries(
    ${MODULE_TARGET} PUBLIC
        ${PROJECT_NAME}_assets
        imgui
        ${PROJECT_NAME}_utils
)
