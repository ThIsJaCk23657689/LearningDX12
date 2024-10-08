﻿cmake_minimum_required (VERSION 3.8)

# define variables
set(MY_PROJECT "Learning DirectX 12")
set(MY_EXECUTABLE "learningdx12")

# 如果支援，則為 MSVC 編譯器啟用熱重新載入。
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(${MY_PROJECT})

add_subdirectory(thirds/DirectX-Headers)
add_executable(${MY_EXECUTABLE} WIN32)

set_target_properties(${MY_EXECUTABLE}
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# define UNICODE marco to support chinese characters.
if(WIN32)
    target_compile_definitions(${MY_EXECUTABLE} PRIVATE UNICODE _UNICODE)
endif()

# 根據不同的編譯器以及組態檔來設定編譯器 flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND CompileOptions -Wall -Wextra -pedantic-errors)
    list(APPEND CompileOptions_Debug -ggdb)
    list(APPEND CompileOptions_Release)
    target_compile_definitions(${MY_EXECUTABLE} PRIVATE $<$<CONFIG:Debug>:_GLIBCXX_DEBUG>)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND CompileOptions -Wall -Wextra -pedantic-errors)
    list(APPEND CompileOptions_Debug)
    list(APPEND CompileOptions_Release)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    list(APPEND CompileOptions /WX /permissive-)
    list(APPEND CompileOptions_Debug)
    list(APPEND CompileOptions_Release)
endif()
target_compile_options(${MY_EXECUTABLE} PRIVATE ${CompileOptions})
target_compile_options(${MY_EXECUTABLE} PRIVATE $<$<CONFIG:Debug>:${CompileOptions_Debug}>)
target_compile_options(${MY_EXECUTABLE} PRIVATE $<$<CONFIG:Release>:${CompileOptions_Release}>)

# 顯示相關資訊
message(STATUS)
message(STATUS "========== Build Information ==========")
message(STATUS "\tHost System Name:         ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "\tHost System Version:      ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "\tHost System Processor:    ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS)
message(STATUS "\tC++ Compiler:             ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "\tC++ Compiler Path:        ${CMAKE_CXX_COMPILER}")
message(STATUS)

if (MSVC)
    message(STATUS "\tMSVC Version:             ${MSVC_VERSION}")
    message(STATUS "\tMSVC Toolset:             ${MSVC_TOOLSET_VERSION}")
endif ()
message(STATUS)
message(STATUS "========================================")

# 處理 submodule -> DirectXMath
set(DIRECTX_MATH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirds/DirectXMath")
add_subdirectory(${DIRECTX_MATH_DIR} "${CMAKE_BINARY_DIR}/DirectXMath")

# 將來源新增到此專案的可執行檔
file(GLOB MY_SOURCE CONFIGURE_DEPENDS
    "src/*.cpp"
    "thirds/imgui/*.cpp"
    "thirds/imgui/backends/imgui_impl_win32.cpp"
    "thirds/imgui/backends/imgui_impl_dx12.cpp"
)
target_sources(${MY_EXECUTABLE} PRIVATE ${MY_SOURCE})

target_include_directories(${MY_EXECUTABLE} PRIVATE 
    "include" 
    "thirds/DirectX-Headers/include/directx"
    "${DIRECTX_MATH_DIR}/Inc"
    "thirds/stb"
    "thirds/imgui"
    "thirds/imgui/backends"
)
target_link_libraries(${MY_EXECUTABLE} PRIVATE 
    DirectX-Headers 
    DirectX-Guids 
    d3d12 
    dxgi 
    d3dcompiler 
    $<$<CONFIG:Debug>:dxguid>
)

# 建立 Symlink 到 assets 資料夾
add_custom_command(TARGET ${MY_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${MY_EXECUTABLE}>/assets"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    COMMENT
        "Creating symlinks to project resources..."
    VERBATIM
)
