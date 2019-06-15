#-------------------------------------------------
#
# Project created by QtCreator 2019-06-08T19:33:09
#
#-------------------------------------------------
#QT      += core gui
#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT        -= core gui
TEMPLATE   = app
TARGET     = sfizz
CONFIG    += link_pkgconfig
PKGCONFIG += alsa freetype2 x11 xext xinerama webkit2gtk-4.0 gtk+-x11-3.0 libcurl
QMAKE_CXXFLAGS += -std=c++17

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS \
           LINUX=1 DEBUG=1 _DEBUG=1 JUCE_APP_VERSION=1.0.0 \
           JUCE_APP_VERSION_HEX=0x10000 \
           JucePlugin_Build_VST=0 JucePlugin_Build_VST3=0 JucePlugin_Build_AU=0 \
           JucePlugin_Build_AUv3=0 JucePlugin_Build_RTAS=0 JucePlugin_Build_AAX=0 \
           JucePlugin_Build_Standalone=1 JucePlugin_Build_Unity=0

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += /usr/include/freetype2 $$_PRO_FILE_PWD_/../../../Source \
               $$_PRO_FILE_PWD_/../../../JuceLibraryCode $(HOME)/JUCE/modules
LIB_BUILDDIR = $$OUT_PWD/../SharedCode
LIBS        += -m64 -ldl -lpthread -lrt -L"$$LIB_BUILDDIR" -lsfizz
QMAKE_CFLAGS = -Wall -Wno-strict-aliasing -Wno-strict-overflow \
               -m64 -O0 -mstackrealign -fPIC -g

HEADERS += ./../../../JuceLibraryCode/AppConfig.h \
           ./../../../JuceLibraryCode/JuceHeader.h \
           ./../../../Source/JuceHelpers.h \
           ./../../../Source/PluginEditor.h \
           ./../../../Source/PluginProcessor.h \
           ./../../../Source/SfzCCEnvelope.h \
           ./../../../Source/SfzContainer.h \
           ./../../../Source/SfzDefaults.h \
           ./../../../Source/SfzEnvelope.h \
           ./../../../Source/SfzFilePool.h \
           ./../../../Source/SfzGlobals.h \
           ./../../../Source/SfzOpcode.h \
           ./../../../Source/SfzRegion.h \
           ./../../../Source/SfzSynth.h \
           ./../../../Source/SfzVoice.h \
           ./../../../Source/StdStringTrimmers.h
SOURCES += ./../../../JuceLibraryCode/include_juce_audio_basics.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_devices.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_formats.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_AAX.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_RTAS_1.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_RTAS_2.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_RTAS_3.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_RTAS_4.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_RTAS_utils.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_Standalone.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_Unity.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_utils.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_VST2.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_plugin_client_VST3.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_processors.cpp \
           ./../../../JuceLibraryCode/include_juce_audio_utils.cpp \
           ./../../../JuceLibraryCode/include_juce_core.cpp \
           ./../../../JuceLibraryCode/include_juce_data_structures.cpp \
           ./../../../JuceLibraryCode/include_juce_events.cpp \
           ./../../../JuceLibraryCode/include_juce_graphics.cpp \
           ./../../../JuceLibraryCode/include_juce_gui_basics.cpp \
           ./../../../JuceLibraryCode/include_juce_gui_extra.cpp \
           ./../../../Source/PluginEditor.cpp \
           ./../../../Source/PluginProcessor.cpp \
           ./../../../Source/SfzRegion.cpp \
           ./../../../Source/SfzSynth.cpp \
           ./../../../Source/SfzVoice.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
