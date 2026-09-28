cmake_minimum_required(VERSION 3.25)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SHADCN_VERSION LIMIT_COUNT 1)
project(shadcn_cpp VERSION ${SHADCN_VERSION} LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SHADCN_BUILD_WIDGETS "Build native Qt widgets" ON)
option(SHADCN_BUILD_MEDIA "Build the optional native video player" OFF)
option(SHADCN_BUILD_TESTS "Build local development tests" OFF)
option(SHADCN_BUILD_EXAMPLES "Build the component gallery" ${PROJECT_IS_TOP_LEVEL})
option(SHADCN_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SHADCN_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)

function(shadcn_target target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    if(NOT DEFINED CMAKE_CXX_STANDARD AND "cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        target_compile_features(${target} PRIVATE cxx_std_26)
    endif()
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(SHADCN_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        # -Wsuggest-override is not implied by -Wall. Without it a missing override compiles
        # cleanly on GCC and only fails the WebAssembly build under clang, which is how a
        # virtual Dialog::open slipped through a local build once.
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsuggest-override)
        if(SHADCN_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        if(SHADCN_SANITIZERS)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
            target_link_options(${target} PUBLIC -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# The component archives link the Qt installation supplied by the application.
add_library(shadcn_core STATIC src/core.cpp)
add_library(shadcn::core ALIAS shadcn_core)
set_target_properties(shadcn_core PROPERTIES EXPORT_NAME core)
shadcn_target(shadcn_core)
target_include_directories(shadcn_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set(shadcn_targets shadcn_core)

if(SHADCN_BUILD_WIDGETS)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)
    add_library(shadcn_widgets STATIC src/widgets.cpp src/overlays.cpp src/controls.cpp src/data.cpp src/navigation.cpp src/feedback.cpp src/rows.cpp
        include/shadcn/widgets.hpp include/shadcn/overlays.hpp include/shadcn/controls.hpp include/shadcn/data.hpp
        include/shadcn/navigation.hpp include/shadcn/feedback.hpp include/shadcn/rows.hpp assets/fonts/fonts.qrc)
    add_library(shadcn::widgets ALIAS shadcn_widgets)
    set_target_properties(shadcn_widgets PROPERTIES AUTOMOC ON AUTORCC ON EXPORT_NAME widgets)
    shadcn_target(shadcn_widgets)
    target_link_libraries(shadcn_widgets PUBLIC shadcn::core Qt6::Widgets)
    list(APPEND shadcn_targets shadcn_widgets)
    if(SHADCN_BUILD_EXAMPLES)
        qt_add_executable(shadcn_gallery examples/gallery/main.cpp examples/gallery/demos.cpp)
        shadcn_target(shadcn_gallery)
        target_link_libraries(shadcn_gallery PRIVATE shadcn::widgets)
        if(SHADCN_BUILD_MEDIA)
            # The media block is defined later, so link it here and let the demos decide
            # at configure time whether the video player is part of the gallery.
            target_link_libraries(shadcn_gallery PRIVATE shadcn::media)
            target_compile_definitions(shadcn_gallery PRIVATE SHADCN_GALLERY_MEDIA=1
                SHADCN_GALLERY_MEDIA_ASSET="${CMAKE_CURRENT_SOURCE_DIR}/assets/media/shadow-sample.mp4")
        endif()
    endif()
endif()

if(SHADCN_BUILD_MEDIA)
    if(NOT SHADCN_BUILD_WIDGETS)
        message(FATAL_ERROR "SHADCN_BUILD_MEDIA requires SHADCN_BUILD_WIDGETS")
    endif()
    # aqtinstall publishes no Qt Multimedia module for wasm_singlethread, so the browser
    # build decodes through emscripten instead. Everything else in the component is
    # unchanged, so both platforms share one widget and one transport.
    if(EMSCRIPTEN)
        set(SHADCN_MEDIA_NATIVE OFF)
    else()
        set(SHADCN_MEDIA_NATIVE ON)
    endif()
    if(SHADCN_MEDIA_NATIVE)
        find_package(Qt6 6.8.2 REQUIRED COMPONENTS Multimedia)
        foreach(module Gui Multimedia)
            if(NOT TARGET Qt6::${module}Private)
                find_package(Qt6 6.8.2 REQUIRED COMPONENTS ${module}Private)
            endif()
        endforeach()
    endif()
    if(SHADCN_MEDIA_NATIVE)
        add_library(shadcn_media STATIC src/media.cpp include/shadcn/media.hpp)
    else()
        # The headers carry the Q_OBJECT classes, so AUTOMOC must see them directly rather
        # than through the .cpp that includes them.
        add_library(shadcn_media STATIC src/media.cpp src/web_playback.cpp
            include/shadcn/media.hpp src/web_playback.hpp)
    endif()
    add_library(shadcn::media ALIAS shadcn_media)
    set_target_properties(shadcn_media PROPERTIES AUTOMOC ON EXPORT_NAME media)
    shadcn_target(shadcn_media)
    if(SHADCN_MEDIA_NATIVE)
        target_link_libraries(shadcn_media PUBLIC shadcn::widgets Qt6::Multimedia
            PRIVATE $<BUILD_INTERFACE:Qt6::GuiPrivate> $<BUILD_INTERFACE:Qt6::MultimediaPrivate>)
    else()
        target_link_libraries(shadcn_media PUBLIC shadcn::widgets)
    endif()
    if(SHADCN_BUILD_EXAMPLES AND SHADCN_MEDIA_NATIVE)
        qt_add_executable(shadcn_video_player examples/video-player/main.cpp)
        shadcn_target(shadcn_video_player)
        target_link_libraries(shadcn_video_player PRIVATE shadcn::media)
    endif()
    install(TARGETS shadcn_media EXPORT shadcnMediaTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(EXPORT shadcnMediaTargets NAMESPACE shadcn:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shadcn)
    install(FILES include/shadcn/media.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shadcn)
endif()

if(SHADCN_BUILD_TESTS)
    enable_testing()
    add_executable(shadcn_core_test tests/core_test.cpp)
    shadcn_target(shadcn_core_test)
    target_link_libraries(shadcn_core_test PRIVATE shadcn::core)
    add_test(NAME core COMMAND shadcn_core_test)
    if(SHADCN_BUILD_WIDGETS)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        add_executable(shadcn_widgets_test tests/widgets_test.cpp)
        set_target_properties(shadcn_widgets_test PROPERTIES AUTOMOC ON)
        shadcn_target(shadcn_widgets_test)
        target_link_libraries(shadcn_widgets_test PRIVATE shadcn::widgets Qt6::Test)
        add_test(NAME widgets COMMAND shadcn_widgets_test)
        set_tests_properties(widgets PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        add_executable(shadcn_overlays_test tests/overlays_test.cpp)
        set_target_properties(shadcn_overlays_test PROPERTIES AUTOMOC ON)
        shadcn_target(shadcn_overlays_test)
        target_link_libraries(shadcn_overlays_test PRIVATE shadcn::widgets Qt6::Test)
        add_test(NAME overlays COMMAND shadcn_overlays_test)
        set_tests_properties(overlays PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        foreach(group IN ITEMS controls data navigation feedback rows)
            add_executable(shadcn_${group}_test tests/${group}_test.cpp)
            set_target_properties(shadcn_${group}_test PROPERTIES AUTOMOC ON)
            shadcn_target(shadcn_${group}_test)
            target_link_libraries(shadcn_${group}_test PRIVATE shadcn::widgets Qt6::Test)
            add_test(NAME ${group} COMMAND shadcn_${group}_test)
            set_tests_properties(${group} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        endforeach()
        if(SHADCN_BUILD_MEDIA)
            # The media cases decode real frames, so they need a fixture. Generate it once at
            # configure time when ffmpeg is present; otherwise the suite reports the gap
            # instead of silently passing.
            set(shadcn_media_fixture "${CMAKE_CURRENT_BINARY_DIR}/media/fixture.mkv")
            find_program(SHADCN_FFMPEG ffmpeg)
            if(SHADCN_FFMPEG)
                file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/media")
                file(GLOB shadcn_media_subtitles "${CMAKE_CURRENT_SOURCE_DIR}/tests/media/captions.srt")
                if(shadcn_media_subtitles)
                    execute_process(
                        COMMAND "${SHADCN_FFMPEG}" -y -loglevel error
                            -f lavfi -i testsrc2=size=320x180:rate=24:duration=6
                            -f lavfi -i sine=frequency=440:duration=6
                            -i "${shadcn_media_subtitles}"
                            -c:v libvpx-vp9 -b:v 300k -c:a libopus -b:a 64k -c:s srt
                            -metadata:s:s:0 language=eng
                            "${shadcn_media_fixture}"
                        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                        RESULT_VARIABLE shadcn_media_result
                        OUTPUT_QUIET ERROR_QUIET)
                    if(NOT shadcn_media_result EQUAL 0)
                        file(REMOVE "${shadcn_media_fixture}")
                    endif()
                endif()
            endif()
            add_executable(shadcn_media_test tests/media_test.cpp)
            set_target_properties(shadcn_media_test PROPERTIES AUTOMOC ON)
            shadcn_target(shadcn_media_test)
            target_link_libraries(shadcn_media_test PRIVATE shadcn::media Qt6::Test)
            add_test(NAME media COMMAND shadcn_media_test)
            # The offscreen plugin decodes media but cannot present frames, so the cases that
            # need a real renderer report a skip. Point this at a virtual display such as
            # Xvfb to run the whole suite: cmake -DSHADCN_MEDIA_PLATFORM=xcb.
            set(SHADCN_MEDIA_PLATFORM "offscreen" CACHE STRING
                "Qt platform plugin for the media suite")
            set_tests_properties(media PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=${SHADCN_MEDIA_PLATFORM};SHADCN_TEST_VIDEO=${shadcn_media_fixture};SHADCN_TEST_CAPTIONS=${shadcn_media_fixture};SHADCN_TEST_CAPTURE=${CMAKE_CURRENT_BINARY_DIR}/media/capture.png"
                TIMEOUT 900)
        endif()
    endif()
endif()

install(TARGETS ${shadcn_targets} EXPORT shadcnTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/shadcn/core.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shadcn)
if(SHADCN_BUILD_WIDGETS)
    install(FILES include/shadcn/shadcn.hpp include/shadcn/widgets.hpp include/shadcn/overlays.hpp include/shadcn/controls.hpp include/shadcn/data.hpp include/shadcn/navigation.hpp include/shadcn/feedback.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shadcn)
endif()
install(EXPORT shadcnTargets NAMESPACE shadcn:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shadcn)
configure_package_config_file(cmake/shadcnConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/shadcnConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shadcn)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/shadcnConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY ExactVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/shadcnConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/shadcnConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shadcn)
install(FILES LICENSE LICENSES/shadcn-MIT.txt LICENSES/Geist-OFL.txt LICENSES/ui-components-MIT.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/shadcn-cpp)
