cmake_minimum_required(VERSION 3.12)
project(c_http_app C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_CXX_STANDARD_LIBRARIES "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "-Weverything -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -pedantic")
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_C_FLAGS_RELEASE "-O3")

    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic")
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_C_FLAGS_RELEASE "-O3")

    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # The flags need to use the exact case specified in documentation, otherwise
    # CMake will not set them properly, even though neither CL nor LINK care
    # about case. For example, `/incremental:no` doesn't work, but
    # `/INCREMENTAL:NO` does.

    set(CMAKE_C_FLAGS "/GS- /Oi /W4")
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_C_FLAGS_RELEASE "/O2")

    set(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL:NO /MANIFEST:NO")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
endif()

add_executable(app app.c)

if(WIN32)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT app)
    target_link_libraries(app PRIVATE ws2_32.lib)
endif()
