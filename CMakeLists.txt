cmake_minimum_required(VERSION 3.22)

project(VocalStrip VERSION 1.0.0 LANGUAGES C CXX)

# Static MSVC runtime: no Visual C++ Redistributable needed on the user PC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED JUCE_LOCAL_PATH)
    add_subdirectory(${JUCE_LOCAL_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(VS_BUILD_TESTS "Build the offline test runner" OFF)

juce_add_plugin(VocalStrip
    COMPANY_NAME                "ROHIT JI"
    BUNDLE_ID                   com.rohitji.vocalstrip
    PLUGIN_MANUFACTURER_CODE    Rhji
    PLUGIN_CODE                 Vcst
    PRODUCT_NAME                "Vocal Channel Strip"
    DESCRIPTION                 "Vocal channel strip: EQ, compressor, de-esser, saturation, glue, delay, reverb"
    FORMATS                     VST3
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx EQ Dynamics)

set(VS_SOURCES Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_sources(VocalStrip PRIVATE ${VS_SOURCES})

set(VS_DEFINITIONS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
if(WIN32)
    list(APPEND VS_DEFINITIONS WINVER=0x0A00 _WIN32_WINNT=0x0A00 JUCE_USE_WINDOWS_MEDIA_FORMAT=0)
endif()
if(UNIX AND NOT APPLE)
    list(APPEND VS_DEFINITIONS JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XINPUT=0
         JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0)
endif()
target_compile_definitions(VocalStrip PUBLIC ${VS_DEFINITIONS})

target_link_libraries(VocalStrip
    PRIVATE juce::juce_audio_processors juce::juce_dsp juce::juce_gui_basics
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(VS_BUILD_TESTS)
    juce_add_console_app(VSTests PRODUCT_NAME "VSTests")
    target_sources(VSTests PRIVATE ${VS_SOURCES} Tests/TestMain.cpp)
    target_compile_definitions(VSTests PRIVATE ${VS_DEFINITIONS} JucePlugin_Name="Vocal Channel Strip"
        JucePlugin_Build_VST3=0 VS_TEST_BUILD=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_include_directories(VSTests PRIVATE Source)
    target_link_libraries(VSTests PRIVATE juce::juce_audio_processors juce::juce_dsp juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
