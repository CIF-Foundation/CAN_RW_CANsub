cmake_minimum_required(VERSION 3.16)

project(cansub_lv_wrapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cansub_lv_wrapper SHARED
    src/cansub_lv.cpp
)

target_include_directories(cansub_lv_wrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(cansub_lv_wrapper PRIVATE CANSUB_LV_BUILD)

set_target_properties(cansub_lv_wrapper PROPERTIES
    OUTPUT_NAME cansub_lv_wrapper
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
    target_compile_options(cansub_lv_wrapper PRIVATE /W4)
else()
    target_compile_options(cansub_lv_wrapper PRIVATE -Wall -Wextra -fPIC)
    set_target_properties(cansub_lv_wrapper PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Keep single-config and multi-config generators writing the DLL/.so to build/bin.
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(cansub_lv_wrapper PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin"
        "LIBRARY_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin"
        "ARCHIVE_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/lib"
    )
endforeach()

add_executable(cansub_lv_smoke tests/smoke.cpp)
target_link_libraries(cansub_lv_smoke PRIVATE cansub_lv_wrapper)
set_target_properties(cansub_lv_smoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(cansub_lv_smoke PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()
if(UNIX)
    set_target_properties(cansub_lv_smoke PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()
