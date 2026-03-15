cmake_minimum_required(VERSION 3.22)
project(Sqeletor VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(/Users/chris/src/github/JUCE JUCE)

juce_add_plugin(Sqeletor
    COMPANY_NAME             "CorvidAudio"
    PLUGIN_MANUFACTURER_CODE Cvda
    PLUGIN_CODE              Sqlt
    FORMATS                  AU
    PRODUCT_NAME             "Sqeletor"
    BUNDLE_ID                "com.CorvidAudio.Sqeletor"
    IS_MIDI_EFFECT           TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        TRUE
)

juce_generate_juce_header(Sqeletor)

target_sources(Sqeletor PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(Sqeletor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Sqeletor
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
