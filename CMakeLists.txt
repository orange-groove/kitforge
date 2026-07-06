cmake_minimum_required(VERSION 3.22)

# macOS deployment + arch (before project() so they apply to JUCE too).
if(APPLE)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0"
            CACHE STRING "macOS deployment target" FORCE)
    endif()

    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64"
            CACHE STRING "macOS architectures" FORCE)
    endif()
endif()

# Static MSVC runtime — installers do not require the VC++ redistributable.
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        CACHE STRING "MSVC runtime linkage" FORCE)
endif()

project(KitForge VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KITFORGE_COPY_PLUGIN_AFTER_BUILD
    "Copy AU/VST3 to system plugin folders after each build (slower dev iteration)"
    OFF)

option(KITFORGE_USE_DEV_SERVER
    "Load React UI from Vite dev server at http://localhost:5173"
    OFF)

option(KITFORGE_BUILD_UI
    "Run npm install && npm run build in ui/ before compiling KitForge"
    ON)

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
    NEEDS_WEBVIEW2 TRUE
    PLUGIN_MANUFACTURER_CODE KtFg
    PLUGIN_CODE Kitf
    BUNDLE_ID com.kitforge.app
    FORMATS AU VST3 Standalone
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "KitForge uses Bluetooth to connect wireless MIDI controllers and drum pads."
    PRODUCT_NAME "KitForge")

juce_generate_juce_header(KitForge)

target_sources(KitForge
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Core/KitForgeServices.cpp
        Source/Core/KitForgeSettings.cpp
        Source/Core/DemoKitSampleBindings.cpp
        Source/Core/KitImportService.cpp
        Source/Models/KitModel.cpp
        Source/Models/KitPieceAddPlacement.cpp
        Source/Models/KitModelHistory.cpp
        Source/Models/SampleIndex.cpp
        Source/Models/SampleIndexService.cpp
        Source/Models/KitValidationService.cpp
        Source/Models/InstalledLibrary.cpp
        Source/Engine/SampleLoader.cpp
        Source/Engine/DrumVoice.cpp
        Source/Engine/ChokeGroupManager.cpp
        Source/Engine/DrumSamplerEngine.cpp
        Source/Midi/MidiLearnManager.cpp
        Source/Serialization/KitSerializer.cpp
        Source/Serialization/KitForgePackageReader.cpp
        Source/Serialization/KitForgePackageWriter.cpp
        Source/Serialization/KitPackageJsonSerializer.cpp
        Source/Serialization/ManifestSerializer.cpp
        Source/Serialization/KitMigrationService.cpp
        Source/Importers/SFZImporter.cpp
        Source/Importers/SfzDrumMappingResolver.cpp
        Source/Importers/SampleNameParser.cpp
        Source/Importers/VendorSampleNaming.cpp
        Source/Importers/KitModelBuilder.cpp
        Source/Importers/LlmSampleClassifier.cpp
        Source/Importers/KitImportLayoutEnforcer.cpp
        Source/Importers/LooseSampleFolderImporter.cpp
        Source/Importers/SFZScanner.cpp
        Source/Importers/ImportResult.cpp
        Source/Importers/SampleLibraryMapper.cpp
        Source/Importers/SampleSwapService.cpp
        Source/Tests/SampleNameParserTests.cpp
        Source/AI/KitRecipe.cpp
        Source/AI/KitRecipeValidator.cpp
        Source/AI/KitModelRecipeBridge.cpp
        Source/AI/KitShellLayoutEnforcer.cpp
        Source/AI/KitMidiMapEnforcer.cpp
        Source/AI/OpenAIKitRecipeClient.cpp
        Source/AI/OpenAIChat.cpp
        Source/AI/AIKitBuilderService.cpp
        Source/UI/DrumPieceComponent.cpp
        Source/UI/KitCanvas.cpp
        Source/UI/KitBuilderPanel.cpp
        Source/UI/InstalledLibrariesPanel.cpp
        Source/UI/LibraryBrowserPanel.cpp
        Source/UI/ImportPreviewPanel.cpp
        Source/UI/SampleMappingPanel.cpp
        Source/UI/LibraryImportFlow.cpp
        Source/UI/MixerPanel.cpp
        Source/UI/AIBuilderPanel.cpp
        Source/UI/SettingsPanel.cpp
        Source/UI/WebViewBridge.cpp
        Source/Standalone/KitForgeStandaloneOptions.cpp)

target_compile_definitions(KitForge
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        KITFORGE_USE_DEV_SERVER=$<IF:$<BOOL:${KITFORGE_USE_DEV_SERVER}>,1,0>)

target_link_libraries(KitForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_gui_extra
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

    add_custom_command(TARGET KitForge_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "$<TARGET_FILE_DIR:KitForge_Standalone>/../Resources/ui/dist"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:KitForge_Standalone>/../Resources/ui/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui/dist"
            "$<TARGET_FILE_DIR:KitForge_Standalone>/../Resources/ui/dist"
        COMMENT "Copying React UI into Standalone app bundle"
        VERBATIM)
endif()

# Embed the React UI inside the plugin bundles so the WebView can load it when
# hosted in a DAW (it resolves Contents/Resources/ui/dist relative to the plugin binary).
foreach(_kitforge_fmt VST3 AU)
    if (TARGET KitForge_${_kitforge_fmt})
        add_custom_command(TARGET KitForge_${_kitforge_fmt} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf
                "$<TARGET_FILE_DIR:KitForge_${_kitforge_fmt}>/../Resources/ui/dist"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:KitForge_${_kitforge_fmt}>/../Resources/ui/dist"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/ui/dist"
                "$<TARGET_FILE_DIR:KitForge_${_kitforge_fmt}>/../Resources/ui/dist"
            COMMENT "Copying React UI into ${_kitforge_fmt} plugin bundle"
            VERBATIM)
    endif()
endforeach()

find_program(NPM_EXECUTABLE npm)

if(KITFORGE_BUILD_UI)
    if(NOT NPM_EXECUTABLE)
        message(WARNING "KitForge: npm not found — React UI will not be built. Install Node.js or set -DKITFORGE_BUILD_UI=OFF")
    else()
        set(_kitforge_ui_stamp "${CMAKE_BINARY_DIR}/kitforge-ui.stamp")

        file(GLOB_RECURSE _kitforge_ui_sources CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/ui/src/*.ts"
            "${CMAKE_SOURCE_DIR}/ui/src/*.tsx"
            "${CMAKE_SOURCE_DIR}/ui/src/*.css"
            "${CMAKE_SOURCE_DIR}/ui/src/assets/*")

        add_custom_command(
            OUTPUT "${_kitforge_ui_stamp}"
            COMMAND ${NPM_EXECUTABLE} install
            COMMAND ${NPM_EXECUTABLE} run build
            COMMAND ${CMAKE_COMMAND} -E touch "${_kitforge_ui_stamp}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/ui"
            DEPENDS
                ${_kitforge_ui_sources}
                "${CMAKE_SOURCE_DIR}/ui/package.json"
                "${CMAKE_SOURCE_DIR}/ui/package-lock.json"
                "${CMAKE_SOURCE_DIR}/ui/vite.config.ts"
                "${CMAKE_SOURCE_DIR}/ui/tsconfig.json"
                "${CMAKE_SOURCE_DIR}/ui/index.html"
            COMMENT "Building React UI (ui/dist)"
            VERBATIM)

        add_custom_target(KitForge_UI_Build DEPENDS "${_kitforge_ui_stamp}")
        add_dependencies(KitForge KitForge_UI_Build)

        if(TARGET KitForge_Standalone)
            add_dependencies(KitForge_Standalone KitForge_UI_Build)
            # Relink Standalone when the UI stamp changes so POST_BUILD recopies ui/dist.
            target_sources(KitForge_Standalone PRIVATE "${_kitforge_ui_stamp}")
        endif()

        foreach(_kitforge_fmt VST3 AU)
            if(TARGET KitForge_${_kitforge_fmt})
                add_dependencies(KitForge_${_kitforge_fmt} KitForge_UI_Build)
                # Relink so POST_BUILD recopies ui/dist when the UI stamp changes.
                target_sources(KitForge_${_kitforge_fmt} PRIVATE "${_kitforge_ui_stamp}")
            endif()
        endforeach()
    endif()
endif()
