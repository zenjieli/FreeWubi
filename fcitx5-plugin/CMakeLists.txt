cmake_minimum_required(VERSION 3.16)
project(freewubi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Treat all compiler warnings as errors
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Default to Debug if no build type is set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Fcitx5Core REQUIRED)

# Find the system fcitx5 addon directory (where libpinyin.so etc. live)
find_path(FCITX_SYSTEM_ADDON_DIR
    NAMES libpinyin.so
    PATHS /usr/lib/x86_64-linux-gnu/fcitx5
          /usr/lib64/fcitx5
          /usr/lib/fcitx5
    NO_DEFAULT_PATH)
if(NOT FCITX_SYSTEM_ADDON_DIR)
    message(FATAL_ERROR "Cannot find system fcitx5 addon directory")
endif()

# User-local addon directory
set(FCITX_USER_ADDON_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/fcitx5")

# Shared dictionary library (no Fcitx5 dependency)
add_library(wubi_dict STATIC wubi_dict.cpp pinyin_dict.cpp)
set_target_properties(wubi_dict PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(wubi_dict PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Portable engine logic (no Fcitx5 dependency)
add_library(engine_core STATIC engine_logic.cpp)
set_target_properties(engine_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(engine_core wubi_dict)
target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Fcitx5 plugin
add_library(freewubi MODULE freewubi.cpp)
set_target_properties(freewubi PROPERTIES PREFIX "")
target_link_libraries(freewubi Fcitx5::Core engine_core)
target_include_directories(freewubi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Install the shared library
install(TARGETS freewubi DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

# Install addon config
install(FILES freewubi-addon.conf.in RENAME freewubi.conf
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Install input method config
install(FILES inputmethod/freewubi.conf.in RENAME freewubi.conf
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")

# Install dictionary data
install(FILES data/wubi86_jidian.dict.yaml
                data/wubi86_jidian_common.dict.yaml
                data/frequency.txt
                data/pinyin.txt
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")

# Generate environment.d config so fcitx5 finds the addon library
configure_file(environment.d.in fcitx5-wubee.conf @ONLY)

install(CODE "
    file(MAKE_DIRECTORY \"\$ENV{HOME}/.config/environment.d\")
    file(INSTALL DESTINATION \"\$ENV{HOME}/.config/environment.d\"
        TYPE FILE FILES \"${CMAKE_CURRENT_BINARY_DIR}/fcitx5-wubee.conf\")
    message(STATUS \"Installed: \$ENV{HOME}/.config/environment.d/fcitx5-wubee.conf\")
")

# Tests
if(BUILD_TESTING)
    enable_testing()
    set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE PATH "Test data directory")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests tests)
endif()
