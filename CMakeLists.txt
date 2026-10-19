cmake_minimum_required(VERSION 3.22)

project(DA_MAQAM_ANALYZER VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE - fetched via CPM / FetchContent
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DA_Maqam_Analyzer
    COMPANY_NAME "D.A Production"
    COMPANY_WEBSITE "https://daproduction.audio"
    COMPANY_EMAIL "info@daproduction.audio"
    PLUGIN_MANUFACTURER_CODE DaPr
    PLUGIN_CODE DaMa
    FORMATS VST3 Standalone
    PRODUCT_NAME "D.A Maqam Analyzer"
    BUNDLE_ID com.daproduction.damaqamanalyzer
    PLUGIN_NAME "D.A Maqam Analyzer"
    DESCRIPTION "Production-grade maqam/dastgah/makam analyzer for monophonic vocal - insert analyzer"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Analyzer
    VST3_AUTO_MANIFEST TRUE
)

juce_generate_juce_header(DA_Maqam_Analyzer)

target_sources(DA_Maqam_Analyzer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/YinPitchDetector.cpp
    Source/DSP/PitchTracker.cpp
    Source/Maqam/MaqamDatabase.cpp
    Source/Maqam/MaqamAnalyzer.cpp
    Source/UI/CustomLookAndFeel.cpp
    Source/Utils/StateManager.cpp
)

target_compile_definitions(DA_Maqam_Analyzer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(DA_Maqam_Analyzer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_data_structures
)

juce_add_binary_data(DA_Maqam_BinaryData SOURCES
    Resources/Images/oud_silhouette.svg
    Resources/Data/maqam_database.json
    Resources/Data/tuning_presets.json
)

target_link_libraries(DA_Maqam_Analyzer PRIVATE DA_Maqam_BinaryData)

# Compiler warnings
if(MSVC)
    target_compile_options(DA_Maqam_Analyzer PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DA_Maqam_Analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests executable (optional)
if(BUILD_TESTS)
    add_subdirectory(Tests)
endif()

# Installation
install(TARGETS DA_Maqam_Analyzer_VST3
    DESTINATION "VST3"
)
