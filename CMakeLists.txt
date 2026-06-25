cmake_minimum_required(VERSION 3.22)

project(KitForge VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KITFORGE_COPY_PLUGIN_AFTER_BUILD
    "Copy AU/VST3 to system plugin folders after each build (slower dev iteration)"
    OFF)

include(FetchContent)

if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(KitForge
    COMPANY_NAME "KitForge"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${KITFORGE_COPY_PLUGIN_AFTER_BUILD}
    NEEDS_CURL TRUE
    PLUGIN_MANUFACTURER_CODE KtFg
    PLUGIN_CODE Kitf
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "KitForge")

juce_add_binary_data(KitForgeDemoSamples
    HEADER_NAME DemoSampleBinaryData.h
    NAMESPACE DemoSampleBinary
    SOURCES
        Resources/demo-samples/kick_center.wav
        Resources/demo-samples/snare_center.wav
        Resources/demo-samples/snare_rimshot.wav
        Resources/demo-samples/rack_tom_hit.wav
        Resources/demo-samples/floor_tom_hit.wav
        Resources/demo-samples/hihat_closed.wav
        Resources/demo-samples/hihat_open.wav
        Resources/demo-samples/crash_hit.wav
        Resources/demo-samples/ride_bow.wav)

juce_add_binary_data(KitForgeResources
    HEADER_NAME KitForgeResources.h
    NAMESPACE KitForgeResources
    SOURCES
        Resources/catalog_manifest.json)

juce_generate_juce_header(KitForge)

target_sources(KitForge
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Core/KitForgeServices.cpp
        Source/Core/KitForgeSettings.cpp
        Source/Core/DemoKitFactory.cpp
        Source/Core/DemoSampleAssets.cpp
        Source/Core/DemoKitSampleBindings.cpp
        Source/Models/KitModel.cpp
        Source/Models/SampleIndex.cpp
        Source/Engine/SampleLoader.cpp
        Source/Engine/DrumVoice.cpp
        Source/Engine/ChokeGroupManager.cpp
        Source/Engine/DrumSamplerEngine.cpp
        Source/Midi/MidiLearnManager.cpp
        Source/Serialization/KitSerializer.cpp
        Source/Catalog/CatalogService.cpp
        Source/Catalog/DownloadManager.cpp
        Source/Catalog/DrumLibraryCatalogManifest.cpp
        Source/Catalog/DrumLibraryCatalogService.cpp
        Source/Catalog/DrumLibraryDownloadManager.cpp
        Source/Catalog/KitInstaller.cpp
        Source/Importers/SFZImporter.cpp
        Source/Importers/KitForgePackImporter.cpp
        Source/Importers/SampleNameParser.cpp
        Source/Importers/KitModelBuilder.cpp
        Source/Importers/KontaktSampleNameParser.cpp
        Source/Importers/KontaktImporter.cpp
        Source/Importers/KitImportLayoutEnforcer.cpp
        Source/Importers/LooseSampleFolderImporter.cpp
        Source/Importers/SFZScanner.cpp
        Source/Importers/ImportResult.cpp
        Source/Importers/SampleLibraryMapper.cpp
        Source/Models/InstalledLibrary.cpp
        Source/Serialization/KitForgePackWriter.cpp
        Source/Tests/SampleNameParserTests.cpp
        Source/AI/KitRecipe.cpp
        Source/AI/KitRecipeValidator.cpp
        Source/AI/KitModelRecipeBridge.cpp
        Source/AI/KitShellLayoutEnforcer.cpp
        Source/AI/KitMidiMapEnforcer.cpp
        Source/AI/OpenAIKitRecipeClient.cpp
        Source/AI/AIKitBuilderService.cpp
        Source/UI/DrumPieceComponent.cpp
        Source/UI/KitCanvas.cpp
        Source/UI/KitBuilderPanel.cpp
        Source/UI/CatalogPanel.cpp
        Source/UI/OnlineLibraryBrowserPanel.cpp
        Source/UI/DownloadProgressComponent.cpp
        Source/UI/InstalledLibrariesPanel.cpp
        Source/UI/LibraryBrowserPanel.cpp
        Source/UI/ImportPreviewPanel.cpp
        Source/UI/SampleMappingPanel.cpp
        Source/UI/LibraryImportFlow.cpp
        Source/UI/MixerPanel.cpp
        Source/UI/AIBuilderPanel.cpp
        Source/UI/SettingsPanel.cpp
        Source/Standalone/KitForgeStandaloneOptions.cpp
        Source/Standalone/KitForgeStandaloneHeader.cpp)

target_compile_definitions(KitForge
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(KitForge
    PRIVATE
        KitForgeDemoSamples
        KitForgeResources
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(KitForge PRIVATE Source)

if (TARGET KitForge_Standalone)
    target_compile_definitions(KitForge_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
    target_sources(KitForge_Standalone PRIVATE
        Source/Standalone/KitForgeStandaloneApp.cpp
        Source/Standalone/KitForgeStandaloneMenuModel.cpp
        Source/Standalone/KitForgeStandaloneWindow.cpp)
endif()
