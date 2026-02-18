cmake_minimum_required(VERSION 3.20)

include(FetchContent)

set(AIFR3D_JUCE_TAG "8.0.3" CACHE STRING "JUCE version tag for Phase 6 plugin")

FetchContent_Declare(
  juce
  URL https://github.com/juce-framework/JUCE/archive/refs/tags/${AIFR3D_JUCE_TAG}.tar.gz
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(aifr3d_vst3
  COMPANY_NAME "AIFR3D"
  BUNDLE_ID com.aifr3d.vst3
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
  PLUGIN_MANUFACTURER_CODE AIfd
  PLUGIN_CODE Af36
  FORMATS VST3
  PRODUCT_NAME "AIFR3D"
)

target_sources(aifr3d_vst3
  PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/analysis/AnalysisJob.h
    src/analysis/AnalysisTypes.h
    src/analysis/AnalysisService.cpp
    src/analysis/AnalysisService.h
    src/export/ChartRenderer.cpp
    src/export/ChartRenderer.h
    src/export/ReportExporter.cpp
    src/export/ReportExporter.h
)

target_compile_features(aifr3d_vst3 PRIVATE cxx_std_20)

target_link_libraries(aifr3d_vst3
  PRIVATE
    aifr3d_core
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_gui_extra
)

target_compile_definitions(aifr3d_vst3
  PUBLIC
    AIFR3D_PROFILE_ANALYSIS=$<IF:$<CONFIG:Release>,0,1>
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
