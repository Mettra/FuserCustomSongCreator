cmake_minimum_required(VERSION 3.5)

project(Fuser_CustomSongCreator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uasset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fuser_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sha1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hmx_midifile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/custom_song_creator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_win32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_dx11.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.cpp
)

add_executable( ${PROJECT_NAME} WIN32 ${SRCS} )

target_include_directories(Fuser_CustomSongCreator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
target_include_directories(Fuser_CustomSongCreator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")
target_include_directories(Fuser_CustomSongCreator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/")

target_link_libraries(
    Fuser_CustomSongCreator
    "d3d11.lib"
)

set_property(TARGET Fuser_CustomSongCreator PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")