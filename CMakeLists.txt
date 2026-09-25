cmake_minimum_required(VERSION 3.20)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PLUGIN_VERSION LIMIT_COUNT 1)
project(MegaloPoolVDJ VERSION ${PLUGIN_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_SOURCES
    src/Main.cpp
    src/MegaloPoolSource.cpp
    src/MegaloPoolApi.cpp
)

if(WIN32)
    # MegaloPool.dll — WinHTTP et Shell sont fournis par Windows : rien à installer.
    add_library(MegaloPool SHARED ${PLUGIN_SOURCES} src/Platform_win.cpp)
    target_link_libraries(MegaloPool PRIVATE winhttp shell32)
    target_compile_definitions(MegaloPool PRIVATE UNICODE _UNICODE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    # Runtime C++ intégré au .dll : pas de « Visual C++ Redistributable » à installer.
    set_property(TARGET MegaloPool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(APPLE)
    # MegaloPool.bundle — libcurl et CoreFoundation sont fournis par macOS.
    add_library(MegaloPool MODULE ${PLUGIN_SOURCES} src/Platform_mac.cpp)
    set_target_properties(MegaloPool PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION bundle
        MACOSX_BUNDLE_BUNDLE_NAME "MegaloPool"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.megalo-lab.megalopool.virtualdj"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PLUGIN_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLUGIN_VERSION}")
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    target_link_libraries(MegaloPool PRIVATE curl ${CORE_FOUNDATION})
else()
    message(FATAL_ERROR "VirtualDJ n'existe que sous Windows et macOS.")
endif()

target_include_directories(MegaloPool PRIVATE src)
target_compile_definitions(MegaloPool PRIVATE MEGALOPOOL_PLUGIN_VERSION="${PLUGIN_VERSION}")
