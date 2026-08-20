cmake_minimum_required(VERSION 3.22)

# --- Locate the MetaModule Plugin SDK ---
if(NOT "${METAMODULE_SDK_DIR}" STREQUAL "")
    message("METAMODULE_SDK_DIR set by CMake variable: ${METAMODULE_SDK_DIR}")
elseif(DEFINED ENV{METAMODULE_SDK_DIR})
    set(METAMODULE_SDK_DIR "$ENV{METAMODULE_SDK_DIR}")
    message("METAMODULE_SDK_DIR set by environment variable: ${METAMODULE_SDK_DIR}")
else()
    set(METAMODULE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../metamodule-plugin-sdk")
    message("METAMODULE_SDK_DIR set to default: ${METAMODULE_SDK_DIR}")
endif()

include(${METAMODULE_SDK_DIR}/plugin.cmake)

# --- Project definition ---
project(BerserkAudio
    VERSION 2.0.1
    DESCRIPTION "Berserk Audio Plugin for MetaModule"
    LANGUAGES C CXX ASM
)

# --- Build target ---
add_library(BerserkAudio STATIC)

target_sources(BerserkAudio PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/plugin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Scrambler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Xenizer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Combverb.cpp
)

target_include_directories(BerserkAudio PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# --- Output directory ---
if("${INSTALL_DIR}" STREQUAL "")
    set(INSTALL_DIR ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins)
endif()

# --- Create the .mmplugin bundle ---
create_plugin(
    SOURCE_LIB       BerserkAudio
    PLUGIN_NAME      BerserkAudio
    PLUGIN_JSON      ${CMAKE_CURRENT_LIST_DIR}/plugin.json
    SOURCE_ASSETS    ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION      ${INSTALL_DIR}
)
