# Generates the iOS Xcode project. Requires macOS and Xcode:
#   cmake -S ios -B build/ios -G Xcode
#   cmake --build build/ios --config Release -- -sdk iphoneos CODE_SIGNING_ALLOWED=NO

cmake_minimum_required(VERSION 3.24)

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum iOS version")

project(StarFlyer LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STARFLYER_BUILD_NUMBER 1 CACHE STRING "CFBundleVersion; must increase for each TestFlight upload")

include(FetchContent)

# Only the app itself belongs in an Xcode archive, not SDL's static library.
set(CMAKE_XCODE_ATTRIBUTE_SKIP_INSTALL YES)

# Keep the version in step with app/build.gradle.kts.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.20/SDL3-3.4.20.tar.gz
        URL_HASH SHA256=f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f)
FetchContent_MakeAvailable(SDL3)

set(STARFLYER_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp)
include(${STARFLYER_CPP_DIR}/GameSources.cmake)

add_executable(StarFlyer MACOSX_BUNDLE
        ${STARFLYER_GAME_SOURCES}
        IosPlatform.mm
        Assets.xcassets)

set_source_files_properties(IosPlatform.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")

target_include_directories(StarFlyer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${STARFLYER_CPP_DIR})

target_link_libraries(StarFlyer PRIVATE
        SDL3::SDL3-static
        "-framework CoreMotion"
        "-framework UIKit")

set_target_properties(StarFlyer PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER com.woodedisland.starflyer
        MACOSX_BUNDLE_BUNDLE_NAME "Star Flyer"
        MACOSX_BUNDLE_BUNDLE_VERSION ${STARFLYER_BUILD_NUMBER}
        MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
        RESOURCE Assets.xcassets
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.woodedisland.starflyer
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM 922BNWKT7H
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
        XCODE_ATTRIBUTE_SKIP_INSTALL NO
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
