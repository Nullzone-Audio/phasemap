cmake_minimum_required(VERSION 3.22)

project(Phasemap VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Oldest macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()

option(PHASEMAP_BUILD_PLUGIN "Build the plugin" ON)
option(PHASEMAP_BUILD_CLAP   "Also build a CLAP plugin (downloads clap-juce-extensions)" ON)
option(PHASEMAP_BUILD_TESTS  "Build the tests (run with ctest)" ON)
option(PHASEMAP_BUILD_TOOLS  "Build the screenshot tool" OFF)
option(PHASEMAP_NO_LINUX_AUDIO "Linux without ALSA/JACK headers (CI, headless test machines)" OFF)

#===============================================================================
# Dependencies: JUCE from a local folder (-DJUCE_DIR=...) or downloaded automatically
include(FetchContent)
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

#===============================================================================
set(PHASEMAP_DSP_SOURCES
    src/dsp/SpectralCorrelation.cpp
    src/dsp/LoudnessMeter.cpp
    src/dsp/TruePeakDetector.cpp)

set(PHASEMAP_UI_SOURCES
    src/ui/Background.cpp
    src/ui/Controls.cpp
    src/ui/HoverTips.cpp
    src/ui/LoudnessPanel.cpp
    src/ui/MeterViews.cpp
    src/ui/PhasemapEditor.cpp
    src/ui/SettingsMenu.cpp
    src/ui/Theme.cpp
    src/ui/UserSettings.cpp)

set(PHASEMAP_ALL_SOURCES ${PHASEMAP_DSP_SOURCES} ${PHASEMAP_UI_SOURCES} src/plugin/PhasemapProcessor.cpp)

set(PHASEMAP_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
if(PHASEMAP_NO_LINUX_AUDIO)
    list(APPEND PHASEMAP_DEFINITIONS JUCE_ALSA=0 JUCE_JACK=0)
endif()

#===============================================================================
if(PHASEMAP_BUILD_PLUGIN)
    set(PHASEMAP_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND PHASEMAP_FORMATS AU)
    endif()
    if(UNIX AND NOT APPLE)
        list(APPEND PHASEMAP_FORMATS LV2)
    endif()

    juce_add_plugin(Phasemap
        COMPANY_NAME "Nullzone"
        COMPANY_WEBSITE "https://nullzone.org"
        BUNDLE_ID "org.nullzone.phasemap"
        PLUGIN_MANUFACTURER_CODE Nlzn
        PLUGIN_CODE Phmp
        PRODUCT_NAME "Phasemap"
        DESCRIPTION "Stereo phase and loudness meter"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        AU_MAIN_TYPE kAudioUnitType_Effect
        VST3_CATEGORIES Fx Analyzer
        LV2URI "https://nullzone.org/plugins/phasemap"
        FORMATS ${PHASEMAP_FORMATS}
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(Phasemap PRIVATE ${PHASEMAP_ALL_SOURCES})
    target_compile_definitions(Phasemap PUBLIC ${PHASEMAP_DEFINITIONS})
    target_link_libraries(Phasemap
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    if(PHASEMAP_BUILD_CLAP)
        FetchContent_Declare(clap-juce-extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d)   # main, needed for JUCE 8.0.10
        FetchContent_MakeAvailable(clap-juce-extensions)
        clap_juce_extensions_plugin(TARGET Phasemap
            CLAP_ID "org.nullzone.phasemap"
            CLAP_FEATURES audio-effect analyzer stereo mono)
    endif()
endif()

#===============================================================================
# Tests: cmake --build build --target phasemap_tests && ctest --test-dir build
if(PHASEMAP_BUILD_TESTS)
    enable_testing()

    function(phasemap_add_test name)
        juce_add_console_app(${name} PRODUCT_NAME ${name})
        target_sources(${name} PRIVATE ${ARGN})
        target_compile_definitions(${name} PRIVATE ${PHASEMAP_DEFINITIONS})
        target_include_directories(${name} PRIVATE src tests)
        target_link_libraries(${name} PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
        add_test(NAME ${name} COMMAND ${name})
        list(APPEND PHASEMAP_TEST_TARGETS ${name})
        set(PHASEMAP_TEST_TARGETS ${PHASEMAP_TEST_TARGETS} PARENT_SCOPE)
    endfunction()

    phasemap_add_test(LoudnessTests tests/LoudnessTests.cpp src/dsp/LoudnessMeter.cpp src/dsp/TruePeakDetector.cpp)
    phasemap_add_test(CorrelationTests tests/CorrelationTests.cpp src/dsp/SpectralCorrelation.cpp)
    phasemap_add_test(ProcessorTests tests/ProcessorTests.cpp ${PHASEMAP_ALL_SOURCES})
    target_compile_definitions(ProcessorTests PRIVATE JucePlugin_Name="Phasemap")

    add_custom_target(phasemap_tests DEPENDS ${PHASEMAP_TEST_TARGETS})
endif()

#===============================================================================
# Screenshot tool for the README and the website
if(PHASEMAP_BUILD_TOOLS)
    juce_add_console_app(PhasemapScreenshots PRODUCT_NAME PhasemapScreenshots)
    target_sources(PhasemapScreenshots PRIVATE tools/Screenshots.cpp ${PHASEMAP_ALL_SOURCES})
    target_compile_definitions(PhasemapScreenshots PRIVATE ${PHASEMAP_DEFINITIONS} JucePlugin_Name="Phasemap")
    target_include_directories(PhasemapScreenshots PRIVATE src)
    target_link_libraries(PhasemapScreenshots PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
