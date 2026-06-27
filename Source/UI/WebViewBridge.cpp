#include "WebViewBridge.h"

#if JUCE_WEB_BROWSER

#include "../PluginProcessor.h"
#include "../Serialization/KitSerializer.h"
#include "../Serialization/KitForgePackageReader.h"
#include "../Importers/SampleLibraryMapper.h"
#include "../Importers/SampleSwapService.h"
#include "../Models/SampleIndexService.h"
#include "../Models/KitValidationService.h"
#include "../Models/SampleSet.h"
#include "../Models/InstalledLibrary.h"
#include "../Models/DrumPieceTypes.h"
#include "../Core/KitForgePaths.h"
#include "../Core/KitImportService.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/KitImportLayoutEnforcer.h"
#include "../Importers/KitModelBuilder.h"
#include "../Models/SampleIndex.h"
#include <cmath>

namespace
{
    constexpr float kLayoutRefWidth  = 980.0f;
    constexpr float kLayoutRefHeight   = 680.0f;

    float semitonesFromPitchRatio (float ratio)
    {
        return 12.0f * std::log2 (juce::jmax (ratio, 0.001f));
    }

    juce::String mimeTypeForPath (const juce::String& path)
    {
        if (path.endsWithIgnoreCase (".html")) return "text/html";
        if (path.endsWithIgnoreCase (".js"))   return "application/javascript";
        if (path.endsWithIgnoreCase (".css"))  return "text/css";
        if (path.endsWithIgnoreCase (".json")) return "application/json";
        if (path.endsWithIgnoreCase (".svg"))  return "image/svg+xml";
        if (path.endsWithIgnoreCase (".png"))  return "image/png";
        if (path.endsWithIgnoreCase (".woff2")) return "font/woff2";
        if (path.endsWithIgnoreCase (".woff")) return "font/woff";
        if (path.endsWithIgnoreCase (".ttf"))  return "font/ttf";
        return "application/octet-stream";
    }

    juce::File getUiDistDirectory()
    {
        const juce::Array<juce::File> candidates =
        {
            juce::File::getCurrentWorkingDirectory().getChildFile ("ui/dist"),
            juce::File::getSpecialLocation (juce::File::currentApplicationFile)
                .getParentDirectory().getParentDirectory().getChildFile ("Resources/ui/dist"),
            juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                .getParentDirectory().getParentDirectory().getChildFile ("Resources/ui/dist"),
        };

        for (const auto& dir : candidates)
            if (dir.getChildFile ("index.html").existsAsFile())
                return dir;

        return juce::File::getCurrentWorkingDirectory().getChildFile ("ui/dist");
    }

    juce::var messageWithType (const juce::String& type)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("type", type);
        return juce::var (obj);
    }

    bool loadLibraryKit (KitForgeAudioProcessor& processor,
                         const juce::String& kitId,
                         KitModel& libraryKitOut,
                         juce::String& libraryNameOut,
                         juce::String& errorOut)
    {
        for (const auto& lib : processor.getServices().getSampleIndex().getLibraries())
        {
            if (lib.id != kitId)
                continue;

            const auto loaded = KitForgePackageReader::loadInstalledKit (lib.getRoot());

            if (! loaded.success)
            {
                errorOut = loaded.errorMessage;
                return false;
            }

            libraryKitOut = loaded.kit;
            libraryKitOut.resolveSamplePaths (lib.getRoot());
            libraryNameOut = lib.name.isNotEmpty() ? lib.name : loaded.kit.kitName;
            return true;
        }

        errorOut = "Installed kit not found: " + kitId;
        return false;
    }

    juce::var sourceEntryToVar (const SampleLibraryMapper::SourceEntry& entry)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("key", entry.key);
        obj->setProperty ("pieceName", entry.pieceName);
        obj->setProperty ("articulationName", entry.articulationName);
        obj->setProperty ("type", drumPieceTypeToString (entry.type));
        obj->setProperty ("midiNote", entry.midiNote);
        obj->setProperty ("sampleCount", entry.sampleCount);
        return juce::var (obj);
    }

    juce::var targetSlotToVar (const SampleLibraryMapper::TargetSlot& slot)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("key", slot.key);
        obj->setProperty ("label", slot.label);
        obj->setProperty ("pieceId", slot.pieceId);
        obj->setProperty ("articulationId", slot.articulationId);
        return juce::var (obj);
    }

    juce::var mappingToVar (const SampleLibraryMapper::Mapping& mapping)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("sourceKey", mapping.sourceKey);
        obj->setProperty ("targetKey", mapping.targetKey);
        obj->setProperty ("enabled", mapping.enabled);
        return juce::var (obj);
    }

    juce::Array<SampleLibraryMapper::Mapping> mappingsFromVar (const juce::var& mappingsVar)
    {
        juce::Array<SampleLibraryMapper::Mapping> mappings;

        if (auto* arr = mappingsVar.getArray())
        {
            for (const auto& item : *arr)
            {
                SampleLibraryMapper::Mapping mapping;
                mapping.sourceKey = item.getProperty ("sourceKey", {}).toString();
                mapping.targetKey = item.getProperty ("targetKey", {}).toString();
                mapping.enabled = (bool) item.getProperty ("enabled", true);
                mappings.add (mapping);
            }
        }

        return mappings;
    }

    bool isLowLatencyMessageType (const juce::String& type)
    {
        return type == "triggerPiece" || type == "movePiece";
    }

    void sendLibraryApplied (WebViewBridge& bridge,
                             const juce::String& libraryId,
                             const juce::String& libraryName,
                             int appliedCount)
    {
        auto msg = messageWithType ("libraryApplied");
        msg.getDynamicObject()->setProperty ("libraryId", libraryId);
        msg.getDynamicObject()->setProperty ("libraryName", libraryName);
        msg.getDynamicObject()->setProperty ("appliedCount", appliedCount);
        msg.getDynamicObject()->setProperty ("message",
                                               "Mapped " + juce::String (appliedCount)
                                                   + " articulations from " + libraryName);
        bridge.sendToWeb (msg);
    }
}

WebViewBridge::WebViewBridge (KitForgeAudioProcessor& processorIn)
    : processor (processorIn),
      loadKitFileChooser ("Load kit", juce::File(), "*.json"),
      saveKitFileChooser ("Save kit", juce::File(), "*.json"),
      importSfzFileChooser ("Import SFZ", juce::File(), "*.sfz"),
      importFolderFileChooser ("Import sample folder", juce::File(), "*"),
      importKitforgeFileChooser ("Install KitForge package", juce::File(), "*.kitforge"),
      assignSampleFileChooser ("Assign sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3")
{
    processor.getKitModel().addListener ([this] { onModelChanged(); });
}

void WebViewBridge::attachToBrowser (juce::WebBrowserComponent& browserIn)
{
    browser = &browserIn;
}

void WebViewBridge::setCanvasSize (float width, float height)
{
    canvasWidth  = juce::jmax (1.0f, width);
    canvasHeight = juce::jmax (1.0f, height);
}

juce::WebBrowserComponent::Options WebViewBridge::buildBrowserOptions()
{
    using Options = juce::WebBrowserComponent::Options;

    Options options = Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withNativeFunction (kNativeFunctionName,
                             [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                             {
                                 const juce::String jsonText = args.size() > 0 ? args[0].toString() : juce::String();

                                 juce::var parsed;

                                 if (jsonText.isNotEmpty()
                                     && ! juce::JSON::parse (jsonText, parsed).failed())
                                 {
                                     const auto type = parsed.getProperty ("type", {}).toString();

                                     if (isLowLatencyMessageType (type))
                                     {
                                         dispatchMessage (parsed);
                                         completion (juce::var());
                                         return;
                                     }
                                 }

                                 juce::MessageManager::callAsync ([this, jsonText, completion = std::move (completion)]
                                 {
                                     if (jsonText.isNotEmpty())
                                         handleMessageFromWeb (jsonText);

                                     completion (juce::var());
                                 });
                             });

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE && ! KITFORGE_USE_DEV_SERVER
    const auto distRoot = getUiDistDirectory();

    options = options.withResourceProvider (
        [distRoot] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
        {
            juce::String path = url;

            if (path.isEmpty() || path == "/")
                path = "index.html";

            if (path.startsWithChar ('/'))
                path = path.substring (1);

            const auto file = distRoot.getChildFile (path);

            if (! file.existsAsFile())
                return std::nullopt;

            juce::MemoryBlock data;
            file.loadFileAsData (data);

            juce::WebBrowserComponent::Resource resource;
            resource.mimeType = mimeTypeForPath (file.getFileName());
            resource.data.resize (data.getSize());
            std::memcpy (resource.data.data(), data.getData(), data.getSize());
            return resource;
        },
        juce::String ("http://localhost:5173"));
#endif

    return options;
}

void WebViewBridge::handleMessageFromWeb (const juce::String& jsonText)
{
    juce::var parsed;

    if (juce::JSON::parse (jsonText, parsed).failed())
    {
        sendError ("Invalid JSON message from UI");
        return;
    }

    dispatchMessage (parsed);
}

void WebViewBridge::dispatchMessage (const juce::var& message)
{
    const auto type = message.getProperty ("type", {}).toString();

    if (type == "ready")                          handleReady();
    else if (type == "triggerPiece")              handleTriggerPiece (message);
    else if (type == "movePiece")                  handleMovePiece (message);
    else if (type == "resizePiece")               handleResizePiece (message);
    else if (type == "learnMidi")                 handleLearnMidi (message);
    else if (type == "assignSample")              handleAssignSample (message);
    else if (type == "updatePiece")               handleUpdatePiece (message);
    else if (type == "renamePiece")               handleRenamePiece (message);
    else if (type == "setArticulationMidi")       handleSetArticulationMidi (message);
    else if (type == "deletePiece")               handleDeletePiece (message);
    else if (type == "saveKit")                   handleSaveKit();
    else if (type == "loadKit")                   handleLoadKit();
    else if (type == "importSfz")                 handleImportSfz();
    else if (type == "importLooseFolder")         handleImportLooseFolder();
    else if (type == "installKitforge")           handleInstallKitforge();
    else if (type == "removeKit")                 handleRemoveKit (message);
    else if (type == "uninstallLibrary")          handleRemoveKit (message);
    else if (type == "useLibrary")                handleUseLibrary (message);
    else if (type == "loadInstalledKit")          handleLoadInstalledKit (message);
    else if (type == "revealKit")                 handleRevealKit (message);
    else if (type == "getLibraryMapping")         handleGetLibraryMapping (message);
    else if (type == "applyLibraryMapping")       handleApplyLibraryMapping (message);
    else if (type == "searchSampleSets")          handleSearchSampleSets (message);
    else if (type == "previewSampleSet")          handlePreviewSampleSet (message);
    else if (type == "swapSampleSet")             handleSwapSampleSet (message);
    else if (type == "rebuildSampleIndex")        handleRebuildSampleIndex (message);
    else if (type == "aiBuildKit")                handleAiBuildKit (message);
    else                                          sendError ("Unknown message type: " + type);
}

void WebViewBridge::handleReady()
{
    {
        const juce::ScopedLock lock (processor.getModelLock());
        processor.getKitModel().normalizeStandardArticulations();
    }
    pushKitState();
    pushCatalogState();
}

void WebViewBridge::handleTriggerPiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const auto articulationId = message.getProperty ("articulationId", {}).toString();

    if (articulationId.isNotEmpty())
        processor.triggerArticulation (pieceId, articulationId);
    else
        processor.triggerPiece (pieceId);
}

void WebViewBridge::handleMovePiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const float nx = (float) message.getProperty ("x", 0.0);
    const float ny = (float) message.getProperty ("y", 0.0);
    const bool finalize = (bool) message.getProperty ("finalize", false);

    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        // Normalized center coordinates → top-left in layout pixel space.
        piece->x = nx * kLayoutRefWidth  - piece->width * 0.5f;
        piece->y = ny * kLayoutRefHeight - piece->height * 0.5f;
        processor.getKitModel().recordLayoutEdit();

        if (finalize)
            processor.getKitModel().notifyChanged();
    }
}

void WebViewBridge::handleResizePiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const float nw = (float) message.getProperty ("width", 0.08);
    const float nh = (float) message.getProperty ("height", 0.08);

    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        piece->width  = juce::jmax (20.0f, nw * kLayoutRefWidth);
        piece->height = juce::jmax (20.0f, nh * kLayoutRefHeight);
        processor.getKitModel().recordLayoutEdit();
    }
}

void WebViewBridge::handleLearnMidi (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const auto artId   = message.getProperty ("articulationId", {}).toString();

    processor.getMidiLearnManager().startLearning (pieceId, artId);
    wasMidiLearning = true;

    auto msg = messageWithType ("midiLearnStarted");
    msg.getDynamicObject()->setProperty ("pieceId", pieceId);
    msg.getDynamicObject()->setProperty ("articulationId", artId);
    sendToWeb (msg);
}

void WebViewBridge::handleAssignSample (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const auto artId   = message.getProperty ("articulationId", {}).toString();

    assignSampleFileChooser.launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, pieceId, artId] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              const juce::ScopedLock lock (processor.getModelLock());

                              if (auto* piece = processor.getKitModel().findPieceById (pieceId))
                              {
                                  if (artId.isNotEmpty())
                                  {
                                      if (piece->findArticulationById (artId) != nullptr)
                                          processor.getKitModel().assignSingleSample (*piece, file, artId);
                                  }
                                  else
                                  {
                                      processor.getKitModel().assignSingleSample (*piece, file);
                                  }

                                  processor.getKitModel().notifyChanged();
                                  processor.rebuildEngine();

                                  auto msg = messageWithType ("sampleAssigned");
                                  msg.getDynamicObject()->setProperty ("pieceId", pieceId);
                                  msg.getDynamicObject()->setProperty ("articulationId", artId);
                                  msg.getDynamicObject()->setProperty ("fileName", file.getFileName());
                                  sendToWeb (msg);
                              }
                          });
}

void WebViewBridge::handleUpdatePiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        if (auto* obj = message.getDynamicObject())
        {
            if (obj->hasProperty ("volume")) piece->volume = (float) obj->getProperty ("volume");
            if (obj->hasProperty ("pan"))    piece->pan    = (float) obj->getProperty ("pan");
            if (obj->hasProperty ("pitch"))  piece->pitch  = (float) obj->getProperty ("pitch");
            if (obj->hasProperty ("muted"))  piece->muted  = (bool)  obj->getProperty ("muted");
            if (obj->hasProperty ("soloed")) piece->soloed = (bool)  obj->getProperty ("soloed");
        }

        processor.getKitModel().notifyChanged();
    }
}

void WebViewBridge::handleRenamePiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const auto newName = message.getProperty ("name", {}).toString();

    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        piece->name = newName;
        processor.getKitModel().notifyChanged();
    }
}

void WebViewBridge::handleSetArticulationMidi (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();
    const auto artId   = message.getProperty ("articulationId", {}).toString();
    const int note     = juce::jlimit (0, 127, (int) message.getProperty ("midiNote", 0));

    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        if (auto* art = piece->findArticulationById (artId))
        {
            const bool wasPrimary = piece->primaryMidiNote == art->midiNote;
            art->midiNote = note;

            if (wasPrimary)
                piece->primaryMidiNote = note;

            piece->syncMidiNotesFromArticulations();
            processor.getKitModel().notifyChanged();
            processor.rebuildEngine();
        }
    }
}

void WebViewBridge::handleDeletePiece (const juce::var& message)
{
    const auto pieceId = message.getProperty ("pieceId", {}).toString();

    const juce::ScopedLock lock (processor.getModelLock());

    if (processor.getKitModel().removePiece (pieceId))
        processor.rebuildEngine();
}

void WebViewBridge::handleSaveKit()
{
    saveKitFileChooser.launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();

                              if (file == juce::File())
                                  return;

                              if (! file.hasFileExtension ("json"))
                                  file = file.withFileExtension ("json");

                              const juce::ScopedLock lock (processor.getModelLock());

                              if (! KitSerializer::saveKitToFile (processor.getKitModel(), file))
                                  sendError ("Could not save kit file.");
                          });
}

void WebViewBridge::handleLoadKit()
{
    loadKitFileChooser.launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              const juce::ScopedLock lock (processor.getModelLock());

                              if (! KitSerializer::loadKitFromFile (processor.getKitModel(), file))
                              {
                                  sendError ("Could not load kit file.");
                                  return;
                              }

                              processor.rebuildEngine();
                              processor.getKitModel().notifyChanged();
                          });
}

void WebViewBridge::handleImportSfz()
{
    importSfzFileChooser.launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              sendBusy ("Importing " + file.getFileNameWithoutExtension() + "…");

                              juce::Thread::launch ([this, file]
                              {
                                  auto result = processor.getServices().getKitImportService().importSfzFile (
                                      file, processor.getServices().getSFZImporter());

                                  juce::MessageManager::callAsync ([this, result]
                                  {
                                      if (! result.success)
                                      {
                                          sendError (result.errorMessage);
                                          return;
                                      }

                                      auto msg = messageWithType ("kitInstalled");
                                      msg.getDynamicObject()->setProperty ("kitId", result.installed.id);
                                      msg.getDynamicObject()->setProperty ("kitName", result.installed.name);
                                      msg.getDynamicObject()->setProperty ("message",
                                                                           "Imported SFZ as " + result.installed.name);
                                      sendToWeb (msg);
                                      pushCatalogState();
                                  });
                              });
                          });
}

void WebViewBridge::handleImportLooseFolder()
{
    importFolderFileChooser.launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto folder = fc.getResult();

                              if (! folder.isDirectory())
                                  return;

                              sendBusy ("Importing " + folder.getFileName() + "…");

                              juce::Thread::launch ([this, folder]
                              {
                                  auto result = processor.getServices().getKitImportService().importLooseFolder (
                                      folder, processor.getServices().getLooseFolderImporter());

                                  juce::MessageManager::callAsync ([this, result]
                                  {
                                      if (! result.success)
                                      {
                                          sendError (result.errorMessage);
                                          return;
                                      }

                                      auto msg = messageWithType ("kitInstalled");
                                      msg.getDynamicObject()->setProperty ("kitId", result.installed.id);
                                      msg.getDynamicObject()->setProperty ("kitName", result.installed.name);
                                      msg.getDynamicObject()->setProperty ("message",
                                                                           "Imported folder as " + result.installed.name);
                                      sendToWeb (msg);
                                      pushCatalogState();
                                  });
                              });
                          });
}

void WebViewBridge::handleInstallKitforge()
{
    importKitforgeFileChooser.launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              sendBusy ("Installing " + file.getFileNameWithoutExtension() + "…");

                              juce::Thread::launch ([this, file]
                              {
                                  auto result = processor.getServices().getKitImportService().installKitforgeFile (file);

                                  juce::MessageManager::callAsync ([this, result]
                                  {
                                      if (! result.success)
                                      {
                                          sendError (result.errorMessage);
                                          return;
                                      }

                                      auto msg = messageWithType ("kitInstalled");
                                      msg.getDynamicObject()->setProperty ("kitId", result.installed.id);
                                      msg.getDynamicObject()->setProperty ("kitName", result.installed.name);
                                      msg.getDynamicObject()->setProperty ("message",
                                                                           "Installed " + result.installed.name);
                                      sendToWeb (msg);
                                      pushCatalogState();
                                  });
                              });
                          });
}

void WebViewBridge::handleRemoveKit (const juce::var& message)
{
    const auto kitId = message.getProperty ("kitId", {}).toString().isNotEmpty()
                           ? message.getProperty ("kitId", {}).toString()
                           : message.getProperty ("libraryId", {}).toString();

    if (kitId.isEmpty())
    {
        sendError ("Kit id is required.");
        return;
    }

    const auto installPath = KitForgePaths::getKitInstallPath (kitId);

    if (installPath.exists())
        installPath.deleteRecursively();

    processor.getServices().getSampleIndex().scanKitsOnDisk();
    pushCatalogState();

    auto msg = messageWithType ("kitRemoved");
    msg.getDynamicObject()->setProperty ("kitId", kitId);
    sendToWeb (msg);
}

void WebViewBridge::handleLoadInstalledKit (const juce::var& message)
{
    const auto kitId = message.getProperty ("kitId", {}).toString().isNotEmpty()
                           ? message.getProperty ("kitId", {}).toString()
                           : message.getProperty ("libraryId", {}).toString();

    sendBusy ("Loading kit…");

    // Defer the (blocking) load one tick so the busy overlay paints first.
    juce::MessageManager::callAsync ([this, kitId]
    {
        KitModel libraryKit;
        juce::String libraryName;
        juce::String error;

        if (! loadLibraryKit (processor, kitId, libraryKit, libraryName, error))
        {
            sendError (error);
            return;
        }

        {
            const juce::ScopedLock lock (processor.getModelLock());
            processor.getKitModel().importContents (libraryKit);

            for (const auto& lib : processor.getServices().getSampleIndex().getLibraries())
            {
                if (lib.id == kitId)
                {
                    processor.getKitModel().resolveSamplePaths (lib.getRoot());
                    break;
                }
            }

            // Fill any cross-library sampleRefs that didn't resolve against this kit root.
            processor.getServices().getSampleIndexService().resolveKitSampleRefs (processor.getKitModel());

            KitModelBuilder::pruneMixedVoices (processor.getKitModel());
            applyImportKitLayout (processor.getKitModel(), 980.0f, 680.0f);
        }

        processor.rebuildEngine();
        processor.getKitModel().notifyChanged();
        pushKitState();
        sendKitValidation();
    });
}

void WebViewBridge::handleRevealKit (const juce::var& message)
{
    const auto kitId = message.getProperty ("kitId", {}).toString().isNotEmpty()
                           ? message.getProperty ("kitId", {}).toString()
                           : message.getProperty ("libraryId", {}).toString();

    const auto path = KitForgePaths::getKitInstallPath (kitId);

    if (path.exists())
        path.revealToUser();
    else
        sendError ("Kit folder not found.");
}

void WebViewBridge::handleUseLibrary (const juce::var& message)
{
    const auto libraryId = message.getProperty ("libraryId", {}).toString();

    if (libraryId.isEmpty())
    {
        sendError ("Library id is required.");
        return;
    }

    KitModel libraryKit;
    juce::String libraryName;
    juce::String error;

    if (! loadLibraryKit (processor, libraryId, libraryKit, libraryName, error))
    {
        sendError (error);
        return;
    }

    int applied = 0;

    {
        const juce::ScopedLock lock (processor.getModelLock());

        if (processor.getKitModel().getPieces().empty())
        {
            sendError ("Create or load a kit layout first, then use a sample library.");
            return;
        }

        const auto mappings = SampleLibraryMapper::suggestMappings (processor.getKitModel(), libraryKit);
        applied = SampleLibraryMapper::applyMappings (processor.getKitModel(), libraryKit, mappings);

        for (const auto& lib : processor.getServices().getSampleIndex().getLibraries())
        {
            if (lib.id == libraryId)
            {
                processor.getKitModel().resolveSamplePaths (lib.getRoot());
                break;
            }
        }
    }

    processor.rebuildEngine();
    processor.getKitModel().notifyChanged();
    pushKitState();

    if (applied <= 0)
    {
        sendError ("No articulations matched between \"" + libraryName + "\" and your kit layout.");
        return;
    }

    sendLibraryApplied (*this, libraryId, libraryName, applied);
}

void WebViewBridge::handleGetLibraryMapping (const juce::var& message)
{
    const auto libraryId = message.getProperty ("libraryId", {}).toString();

    if (libraryId.isEmpty())
    {
        sendError ("Library id is required.");
        return;
    }

    KitModel libraryKit;
    juce::String libraryName;
    juce::String error;

    if (! loadLibraryKit (processor, libraryId, libraryKit, libraryName, error))
    {
        sendError (error);
        return;
    }

    KitModel targetSnapshot;

    {
        const juce::ScopedLock lock (processor.getModelLock());
        targetSnapshot = processor.getKitModel();
    }

    if (targetSnapshot.getPieces().empty())
    {
        sendError ("Create or load a kit layout first, then map a sample library.");
        return;
    }

    const auto sources = SampleLibraryMapper::listLibrarySources (libraryKit);
    const auto targets = SampleLibraryMapper::listTargetSlots (targetSnapshot);
    const auto suggested = SampleLibraryMapper::suggestMappings (targetSnapshot, libraryKit);

    juce::Array<juce::var> sourceVars;
    juce::Array<juce::var> targetVars;
    juce::Array<juce::var> mappingVars;

    for (const auto& source : sources)
        sourceVars.add (sourceEntryToVar (source));

    for (const auto& target : targets)
        targetVars.add (targetSlotToVar (target));

    for (int i = 0; i < sources.size(); ++i)
    {
        const auto mapping = i < suggested.size()
                                 ? suggested.getReference (i)
                                 : SampleLibraryMapper::Mapping { sources.getReference (i).key, {}, true };
        mappingVars.add (mappingToVar (mapping));
    }

    auto msg = messageWithType ("libraryMappingState");
    msg.getDynamicObject()->setProperty ("libraryId", libraryId);
    msg.getDynamicObject()->setProperty ("libraryName", libraryName);
    msg.getDynamicObject()->setProperty ("kitName", targetSnapshot.kitName);
    msg.getDynamicObject()->setProperty ("sources", sourceVars);
    msg.getDynamicObject()->setProperty ("targets", targetVars);
    msg.getDynamicObject()->setProperty ("mappings", mappingVars);
    sendToWeb (msg);
}

void WebViewBridge::handleApplyLibraryMapping (const juce::var& message)
{
    const auto libraryId = message.getProperty ("libraryId", {}).toString();

    if (libraryId.isEmpty())
    {
        sendError ("Library id is required.");
        return;
    }

    KitModel libraryKit;
    juce::String libraryName;
    juce::String error;

    if (! loadLibraryKit (processor, libraryId, libraryKit, libraryName, error))
    {
        sendError (error);
        return;
    }

    const auto mappings = mappingsFromVar (message.getProperty ("mappings", {}));
    int applied = 0;

    {
        const juce::ScopedLock lock (processor.getModelLock());

        if (processor.getKitModel().getPieces().empty())
        {
            sendError ("Create or load a kit layout first, then map a sample library.");
            return;
        }

        applied = SampleLibraryMapper::applyMappings (processor.getKitModel(), libraryKit, mappings);

        for (const auto& lib : processor.getServices().getSampleIndex().getLibraries())
        {
            if (lib.id == libraryId)
            {
                processor.getKitModel().resolveSamplePaths (lib.getRoot());
                break;
            }
        }
    }

    processor.rebuildEngine();
    processor.getKitModel().notifyChanged();
    pushKitState();

    if (applied <= 0)
    {
        sendError ("No articulations were applied from \"" + libraryName + "\".");
        return;
    }

    sendLibraryApplied (*this, libraryId, libraryName, applied);
}

void WebViewBridge::sendKitValidation()
{
    KitValidationReport report;

    {
        const juce::ScopedLock lock (processor.getModelLock());
        report = KitValidationService::validateKit (processor.getKitModel(),
                                                    processor.getServices().getSampleIndexService());
    }

    if (! report.hasIssues())
        return;

    auto msg = messageWithType ("validationState");
    msg.getDynamicObject()->setProperty ("report", report.toVar());
    sendToWeb (msg);
}

void WebViewBridge::sendSampleIndexState()
{
    auto& service = processor.getServices().getSampleIndexService();

    auto msg = messageWithType ("sampleIndexState");
    msg.getDynamicObject()->setProperty ("sampleSetCount", service.getSampleSetCount());
    msg.getDynamicObject()->setProperty ("libraryCount", service.getLibraryCount());
    msg.getDynamicObject()->setProperty ("missingSampleCount", service.getMissingSampleCount());
    sendToWeb (msg);
}

void WebViewBridge::handleSearchSampleSets (const juce::var& message)
{
    SampleSetQuery query;

    if (auto* q = message.getProperty ("query", {}).getDynamicObject())
    {
        query.instrumentType = q->getProperty ("instrumentType").toString();
        query.articulation   = q->getProperty ("articulation").toString();
        query.libraryId      = q->getProperty ("libraryId").toString();
        query.text           = q->getProperty ("text").toString();
        query.minVelocityLayers = (int) q->getProperty ("minVelocityLayers");
        query.minRoundRobins    = (int) q->getProperty ("minRoundRobins");

        if (auto* tagsArr = q->getProperty ("tags").getArray())
            for (const auto& t : *tagsArr)
                query.tags.add (t.toString());
    }

    juce::Thread::launch ([this, query]
    {
        auto& service = processor.getServices().getSampleIndexService();
        service.rebuildIfStale();
        const auto results = service.searchSampleSets (query);

        juce::Array<juce::var> resultVars;
        for (const auto& set : results)
            resultVars.add (set.toVar());

        juce::MessageManager::callAsync ([this, resultVars]
        {
            auto msg = messageWithType ("sampleSetSearchResults");
            msg.getDynamicObject()->setProperty ("results", resultVars);
            sendToWeb (msg);
            sendSampleIndexState();
        });
    });
}

void WebViewBridge::handlePreviewSampleSet (const juce::var& message)
{
    const auto sampleSetId = message.getProperty ("sampleSetId", {}).toString();

    if (sampleSetId.isEmpty())
        return;

    juce::Thread::launch ([this, sampleSetId]
    {
        auto& service = processor.getServices().getSampleIndexService();
        service.rebuildIfStale();

        juce::String absPath;

        if (const auto set = service.findSampleSetById (sampleSetId))
        {
            if (auto resolved = service.resolveSampleRef (set->previewSampleRef))
                if (resolved->exists)
                    absPath = resolved->absoluteFilePath;

            // Fall back to the first sample's absolute path if the ref didn't resolve.
            if (absPath.isEmpty())
                for (const auto& layer : set->layers)
                    if (! layer.roundRobins.samples.empty())
                    {
                        absPath = layer.roundRobins.samples.front().filePath;
                        break;
                    }
        }

        if (absPath.isEmpty())
            return;

        juce::MessageManager::callAsync ([this, absPath]
        {
            processor.previewSampleFile (absPath, 110.0f);
        });
    });
}

void WebViewBridge::handleSwapSampleSet (const juce::var& message)
{
    const auto sampleSetId = message.getProperty ("sampleSetId", {}).toString();
    const auto targetVar = message.getProperty ("target", {});

    SampleSwapService::Target target;
    target.pieceId        = targetVar.getProperty ("pieceId", {}).toString();
    target.articulationId = targetVar.getProperty ("articulationId", {}).toString();
    target.layerId        = targetVar.getProperty ("layerId", {}).toString();

    const auto modeStr = targetVar.getProperty ("mode", "articulation").toString();
    if (modeStr == "piece")       target.mode = SampleSwapService::Mode::piece;
    else if (modeStr == "layer")  target.mode = SampleSwapService::Mode::layer;
    else                          target.mode = SampleSwapService::Mode::articulation;

    SampleSwapService::Options options;
    if (auto* opt = message.getProperty ("options", {}).getDynamicObject())
    {
        options.useSourceName = (bool) opt->getProperty ("useSourceName");
        options.useSourceMidi = (bool) opt->getProperty ("useSourceMidi");
        options.useSourceVelocityRanges = (bool) opt->getProperty ("useSourceVelocityRanges");
    }

    if (sampleSetId.isEmpty() || target.pieceId.isEmpty())
    {
        auto msg = messageWithType ("sampleSwapFailed");
        msg.getDynamicObject()->setProperty ("message", "Missing swap target or sample set.");
        sendToWeb (msg);
        return;
    }

    sendBusy ("Swapping samples…");

    juce::Thread::launch ([this, sampleSetId, target, options]
    {
        auto& service = processor.getServices().getSampleIndexService();
        service.rebuildIfStale();

        const auto found = service.findSampleSetById (sampleSetId);

        if (! found.has_value())
        {
            juce::MessageManager::callAsync ([this]
            {
                auto msg = messageWithType ("sampleSwapFailed");
                msg.getDynamicObject()->setProperty ("message", "Selected sample set is no longer available.");
                sendToWeb (msg);
            });
            return;
        }

        const SampleSet set = *found; // copy so we can mutate the model on the message thread

        juce::MessageManager::callAsync ([this, set, target, options]
        {
            SampleSwapService::Result result;

            {
                const juce::ScopedLock lock (processor.getModelLock());
                result = SampleSwapService::swap (processor.getKitModel(), set, target, options);

                if (result.success)
                    processor.getServices().getSampleIndexService().resolveKitSampleRefs (processor.getKitModel());
            }

            if (! result.success)
            {
                auto msg = messageWithType ("sampleSwapFailed");
                msg.getDynamicObject()->setProperty ("message", result.errorMessage);
                sendToWeb (msg);
                return;
            }

            processor.rebuildEngine();
            processor.getKitModel().notifyChanged();
            pushKitState();

            auto msg = messageWithType ("sampleSwapCompleted");
            msg.getDynamicObject()->setProperty ("pieceId", target.pieceId);
            msg.getDynamicObject()->setProperty ("articulationId", target.articulationId);
            msg.getDynamicObject()->setProperty ("sampleSetId", set.id);
            sendToWeb (msg);

            sendKitValidation();
        });
    });
}

void WebViewBridge::handleRebuildSampleIndex (const juce::var&)
{
    juce::Thread::launch ([this]
    {
        processor.getServices().getSampleIndexService().rebuildIndex();

        juce::MessageManager::callAsync ([this] { sendSampleIndexState(); });
    });
}

void WebViewBridge::handleAiBuildKit (const juce::var& message)
{
    const auto prompt = message.getProperty ("prompt", {}).toString().trim();

    if (prompt.isEmpty())
    {
        auto msg = messageWithType ("aiBuildComplete");
        msg.getDynamicObject()->setProperty ("success", false);
        msg.getDynamicObject()->setProperty ("message", "AI prompt is empty.");
        sendToWeb (msg);
        return;
    }

    auto& ai = processor.getServices().getAIBuilder();

    if (ai.isBusy())
    {
        auto msg = messageWithType ("aiBuildComplete");
        msg.getDynamicObject()->setProperty ("success", false);
        msg.getDynamicObject()->setProperty ("message", "AI kit build already in progress.");
        sendToWeb (msg);
        return;
    }

    const KitModel* currentKit = nullptr;

    {
        const juce::ScopedLock lock (processor.getModelLock());
        currentKit = &processor.getKitModel();
    }

    ai.submitPrompt (prompt,
                     currentKit,
                     canvasWidth,
                     canvasHeight,
                     [this] (const AIKitBuildResult& result)
                     {
                         juce::MessageManager::callAsync ([this, result]
                         {
                             auto msg = messageWithType ("aiBuildComplete");
                             msg.getDynamicObject()->setProperty ("success", result.success);
                             msg.getDynamicObject()->setProperty ("message", result.message);
                             sendToWeb (msg);

                             if (! result.success)
                                 return;

                             {
                                 const juce::ScopedLock modelLock (processor.getModelLock());
                                 processor.getKitModel().importContents (result.kit);
                             }

                             processor.rebuildEngine();
                             processor.getKitModel().notifyChanged();
                         });
                     });
}

void WebViewBridge::onModelChanged()
{
    juce::MessageManager::callAsync ([this]
    {
        checkMidiLearnTransition();
    });
    schedulePushKitState();
}

void WebViewBridge::schedulePushKitState()
{
    kitStateDirty.store (true, std::memory_order_relaxed);

    if (kitStatePushScheduled.exchange (true))
        return;

    juce::Thread::launch ([this]
    {
        juce::var msg;

        while (kitStateDirty.exchange (false))
            msg = buildKitStateMessage();

        juce::MessageManager::callAsync ([this, msg]
        {
            kitStatePushScheduled.store (false, std::memory_order_relaxed);
            sendToWeb (msg);

            if (kitStateDirty.load (std::memory_order_relaxed))
                schedulePushKitState();
        });
    });
}

void WebViewBridge::checkMidiLearnTransition()
{
    auto& learn = processor.getMidiLearnManager();

    if (wasMidiLearning && ! learn.isLearning())
    {
        const auto pieceId = learn.getTargetPieceId();
        const auto artId   = learn.getTargetArticulationId();

        if (pieceId.isNotEmpty())
        {
            int midiNote = -1;

            {
                const juce::ScopedLock lock (processor.getModelLock());

                if (const auto* piece = processor.getKitModel().findPieceById (pieceId))
                {
                    if (const auto* art = artId.isNotEmpty()
                                              ? piece->findArticulationById (artId)
                                              : piece->getPrimaryArticulation())
                        midiNote = art->midiNote;
                }
            }

            if (midiNote >= 0)
            {
                auto msg = messageWithType ("midiLearnCompleted");
                msg.getDynamicObject()->setProperty ("pieceId", pieceId);
                msg.getDynamicObject()->setProperty ("articulationId", artId);
                msg.getDynamicObject()->setProperty ("midiNote", midiNote);
                sendToWeb (msg);
            }
        }
    }

    wasMidiLearning = learn.isLearning();
}

juce::var WebViewBridge::buildKitStateMessage() const
{
    const juce::ScopedLock lock (processor.getModelLock());
    auto kitVar = KitSerializer::kitToUiVar (processor.getKitModel());

    if (auto* root = kitVar.getDynamicObject())
    {
        root->setProperty ("canvasWidth", kLayoutRefWidth);
        root->setProperty ("canvasHeight", kLayoutRefHeight);

        if (auto* pieces = root->getProperty ("pieces").getArray())
        {
            for (auto& pieceVar : *pieces)
            {
                if (auto* obj = pieceVar.getDynamicObject())
                {
                    const float x = (float) obj->getProperty ("x");
                    const float y = (float) obj->getProperty ("y");
                    const float w = (float) obj->getProperty ("width");
                    const float h = (float) obj->getProperty ("height");
                    const float pitch = (float) obj->getProperty ("pitch");

                    obj->setProperty ("x", x / kLayoutRefWidth);
                    obj->setProperty ("y", y / kLayoutRefHeight);
                    obj->setProperty ("width", w / kLayoutRefWidth);
                    obj->setProperty ("height", h / kLayoutRefHeight);
                    obj->setProperty ("pitchSemitones", semitonesFromPitchRatio (pitch));
                }
            }
        }
    }

    auto msg = messageWithType ("kitState");
    msg.getDynamicObject()->setProperty ("kit", kitVar);
    return msg;
}

juce::var WebViewBridge::buildCatalogStateMessage() const
{
    juce::Array<juce::var> installedKits;

    for (const auto& lib : processor.getServices().getSampleIndex().getLibraries())
        installedKits.add (lib.toVar());

    auto msg = messageWithType ("catalogState");
    msg.getDynamicObject()->setProperty ("installed", installedKits);
    return msg;
}

void WebViewBridge::pushKitState()
{
    schedulePushKitState();
}

void WebViewBridge::pushCatalogState()
{
    sendToWeb (buildCatalogStateMessage());
}

void WebViewBridge::sendError (const juce::String& message)
{
    auto msg = messageWithType ("error");
    msg.getDynamicObject()->setProperty ("message", message);
    sendToWeb (msg);
}

void WebViewBridge::sendBusy (const juce::String& label)
{
    auto msg = messageWithType ("busy");
    msg.getDynamicObject()->setProperty ("label", label);
    sendToWeb (msg);
}

void WebViewBridge::sendToWeb (const juce::var& message)
{
    if (browser == nullptr)
        return;

    browser->emitEventIfBrowserIsVisible (kNativeEventName, message);
}

#endif
