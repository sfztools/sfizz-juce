cmake_minimum_required(VERSION 3.5)
# You need to generate the AppConfig.h and the JuceHeader.h through the Projucer; the rest is more or less here 

project(sfizz VERSION 1.0.0 LANGUAGES CXX)

# Change this if needed
if(UNIX)
    set(JUCE_ROOT_DIR "~/sources/JUCE")
else()
    set(JUCE_ROOT_DIR "c:/JUCE")
endif()
set(ASIO_SDK "C:/Users/paulf/source/ASIOSDK2.3.2/common")


# Set these to match the juce header file, modulo the audio plugin clients
set(JUCE_MODULES
    juce_audio_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_processors
    juce_audio_utils
    juce_dsp
    juce_core
    juce_data_structures
    juce_events
    juce_graphics
    juce_gui_basics
    juce_gui_extra
)

set(SOURCES
    Source/SfzRegion.cpp
    Source/SfzSynth.cpp
    Source/SfzVoice.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp 
)
    
set(TEST_SOURCES
    Source/SfzRegion.cpp
    Source/SfzSynth.cpp
    Source/SfzVoice.cpp
    Tests/RegionBuildTests.cpp
    Tests/RegionActivationTests.cpp
    Tests/CCEnvelopeTest.cpp
    Tests/FileTests.cpp
    Tests/OpcodeTests.cpp
    Tests/Main.cpp
)

# Multicore win32
if(WIN32)
    add_compile_options(/MP)
endif(WIN32)

if (UNIX)
    find_package(Freetype REQUIRED)
    find_package(ALSA REQUIRED)
    find_package(X11 REQUIRED)
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(deps REQUIRED IMPORTED_TARGET gtk+-3.0 glib-2.0 webkit2gtk-4.0)
endif()

# Juce modules as library targets
set(JUCE_MODULES_DIR "${JUCE_ROOT_DIR}/modules")
set(JUCE_STANDALONE_SOURCES ${JUCE_MODULES_DIR}/juce_audio_plugin_client/juce_audio_plugin_client_Standalone.cpp ${JUCE_MODULES_DIR}/juce_audio_plugin_client/juce_audio_plugin_client_utils.cpp)
set(JUCE_VST3_SOURCES ${JUCE_MODULES_DIR}/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp ${JUCE_MODULES_DIR}/juce_audio_plugin_client/juce_audio_plugin_client_utils.cpp)

add_library(JUCE)
# Add sources
foreach(module ${JUCE_MODULES})
    target_sources(JUCE PRIVATE ${JUCE_MODULES_DIR}/${module}/${module}.cpp)
endforeach()

# JUCE includes
target_include_directories(JUCE PUBLIC SYSTEM ${JUCE_MODULES_DIR})
target_include_directories(JUCE PUBLIC JuceLibraryCode)
target_include_directories(JUCE PUBLIC ${JUCE_MODULES_DIR}/juce_audio_processors/format_types/VST3_SDK)

# Compile features
target_compile_features(JUCE PRIVATE cxx_std_17)
if (UNIX)
    target_link_libraries(JUCE ALSA::ALSA Freetype::Freetype X11::X11 X11::Xext X11::Xinerama CURL::libcurl PkgConfig::deps)
    # Force include the app config
    target_compile_options(JUCE PUBLIC -include "AppConfig.h")
endif()
    
if(MSVC)
# Force include the app config
target_compile_options(JUCE PUBLIC /FI AppConfig.h)
target_compile_options(JUCE PUBLIC /DJUCE_ASIO)
target_include_directories(JUCE PRIVATE SYSTEM ${ASIO_SDK})
endif(MSVC)

###############################
# Standalone plugin application
add_executable(${PROJECT_NAME}_Standalone ${SOURCES} ${JUCE_STANDALONE_SOURCES})
# Per OS properties
if(WIN32)
    set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES WIN32_EXECUTABLE True)
    # Required only for Range v3
    target_compile_options(${PROJECT_NAME}_Standalone PRIVATE /permissive-)
endif(WIN32)
if(UNIX)
    target_link_libraries(${PROJECT_NAME}_Standalone ${CMAKE_DL_LIBS} Threads::Threads)
endif(UNIX)
set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES OUTPUT_NAME "sfizz")
target_link_libraries(${PROJECT_NAME}_Standalone JUCE)
target_include_directories(${PROJECT_NAME}_Standalone SYSTEM PRIVATE Includes)
target_compile_features(${PROJECT_NAME}_Standalone PRIVATE cxx_std_17)

###############################
# Test application (No WIN32 here, we want the main entry point and not WinMain)
add_executable(${PROJECT_NAME}_Test ${TEST_SOURCES})
# Per OS properties
if(UNIX)
target_link_libraries(${PROJECT_NAME}_Test ${CMAKE_DL_LIBS} Threads::Threads )
endif(UNIX)
if(WIN32)
# Required only for Range v3
target_compile_options(${PROJECT_NAME}_Test PRIVATE /permissive-)
endif(WIN32)
target_compile_definitions(${PROJECT_NAME}_Test PRIVATE JUCE_STANDALONE_APPLICATION=1)
set_target_properties(${PROJECT_NAME}_Test PROPERTIES OUTPUT_NAME "sfizz_Tests")
target_link_libraries(${PROJECT_NAME}_Test JUCE)
target_include_directories(${PROJECT_NAME}_Test SYSTEM PRIVATE Includes)
target_compile_features(${PROJECT_NAME}_Test PRIVATE cxx_std_17)
file(COPY "Tests" DESTINATION ${CMAKE_BINARY_DIR})

###############################
# VST3 library
add_library(${PROJECT_NAME}_VST SHARED ${SOURCES} ${JUCE_VST3_SOURCES})
if(WIN32)
# Required only for Range v3
target_compile_options(${PROJECT_NAME}_VST PRIVATE /permissive-)
endif(WIN32)
set_target_properties(${PROJECT_NAME}_VST PROPERTIES OUTPUT_NAME "sfizz")
set_target_properties(${PROJECT_NAME}_VST PROPERTIES SUFFIX ".vst3")
target_link_libraries(${PROJECT_NAME}_VST JUCE)
target_include_directories(${PROJECT_NAME}_VST SYSTEM PRIVATE Includes)
target_compile_features(${PROJECT_NAME}_VST PRIVATE cxx_std_17)