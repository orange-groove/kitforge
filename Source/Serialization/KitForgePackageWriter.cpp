#include "KitForgePackageWriter.h"
#include "KitPackageJsonSerializer.h"
#include "ManifestSerializer.h"
#include <map>

namespace
{
    bool zipFolder (const juce::File& sourceFolder, const juce::File& zipFile, juce::String& error)
    {
        juce::ZipFile::Builder builder;

        for (const auto& file : sourceFolder.findChildFiles (juce::File::findFiles, true))
        {
            const auto relativePath = file.getRelativePathFrom (sourceFolder);
            builder.addFile (file, 9, relativePath);
        }

        if (zipFile.existsAsFile() && ! zipFile.deleteFile())
        {
            error = "Could not replace existing package file.";
            return false;
        }

        juce::FileOutputStream stream (zipFile);

        if (stream.failedToOpen())
        {
            error = "Could not open package file for writing.";
            return false;
        }

        if (! builder.writeToStream (stream, nullptr))
        {
            error = "Failed to write zip archive";

            if (stream.getStatus().getErrorMessage().isNotEmpty())
                error += ": " + stream.getStatus().getErrorMessage();

            return false;
        }

        return true;
    }

    juce::String slugify (juce::String text)
    {
        text = text.trim().toLowerCase();
        juce::String out;

        for (auto c : text)
        {
            if (juce::CharacterFunctions::isLetterOrDigit (c))
                out << c;
            else if (c == ' ' || c == '-' || c == '_')
                out << '-';
        }

        while (out.contains ("--"))
            out = out.replace ("--", "-");

        return out.trimCharactersAtStart ("-").trimCharactersAtEnd ("-");
    }

    juce::String sampleSubfolderForPiece (DrumPieceType type)
    {
        switch (type)
        {
            case DrumPieceType::kick:     return "kick";
            case DrumPieceType::snare:    return "snare";
            case DrumPieceType::rackTom:
            case DrumPieceType::floorTom: return "toms";
            case DrumPieceType::hiHat:    return "hats";
            case DrumPieceType::crash:
            case DrumPieceType::ride:
            case DrumPieceType::china:
            case DrumPieceType::splash:   return "cymbals";
            default:                      return "other";
        }
    }

    juce::String uniqueSampleFileName (const juce::String& baseFolder,
                                       const juce::File& sourceFile,
                                       juce::HashMap<juce::String, int>& usedNames)
    {
        const auto key = baseFolder + "/" + sourceFile.getFileName().toLowerCase();

        if (! usedNames.contains (key))
        {
            usedNames.set (key, 1);
            return sourceFile.getFileName();
        }

        const int count = ++usedNames.getReference (key);
        return sourceFile.getFileNameWithoutExtension()
             + "_" + juce::String (count)
             + sourceFile.getFileExtension();
    }

    juce::String defaultCreditsText (const juce::String& kitName)
    {
        return "Kit: " + kitName + "\n"
             + "Packaged by KitForge on "
             + juce::Time::getCurrentTime().toISO8601 (true) + ".\n";
    }

    juce::String defaultLicensePlaceholder()
    {
        return "License information for this kit was not provided.\n"
             "Verify usage rights with the original sample provider before distribution.\n";
    }
}

KitForgePackageWriter::WriteResult KitForgePackageWriter::writeFolder (const KitModel& kit,
                                                                      const juce::File& outputFolder,
                                                                      const WriteOptions& options) const
{
    WriteResult result;
    result.outputFolder = outputFolder;

    if (options.kitName.isEmpty())
    {
        result.errorMessage = "Kit name is required.";
        return result;
    }

    const auto packageId = options.packageId.isNotEmpty() ? options.packageId : slugify (options.kitName);

    if (packageId.isEmpty())
    {
        result.errorMessage = "Could not derive package id.";
        return result;
    }

    outputFolder.createDirectory();
    outputFolder.getChildFile ("samples").createDirectory();
    outputFolder.getChildFile ("artwork").createDirectory();
    outputFolder.getChildFile ("preview").createDirectory();

    KitModel kitCopy;
    kitCopy.importContents (kit);
    kitCopy.kitName = options.kitName;

    const bool selfContained = options.referenceMode == ReferenceMode::selfContained;

    // Tally dependency usage per source library (from sampleRefs) for credits.
    std::map<juce::String, int> libraryUsage;

    for (const auto& piece : kitCopy.getPieces())
        for (const auto& art : piece.articulations)
            for (const auto& layer : art.layers)
                for (const auto& sample : layer.roundRobins.samples)
                    if (sample.sampleRef.libraryId.isNotEmpty())
                        ++libraryUsage[sample.sampleRef.libraryId];

    juce::HashMap<juce::String, int> usedNames;
    juce::HashMap<juce::String, juce::String> copiedSourcePaths;
    juce::StringArray pieceIds;

    for (const auto& piece : kitCopy.getPieces())
        pieceIds.add (piece.id);

    for (const auto& pieceId : pieceIds)
    {
        auto* piece = kitCopy.findPieceById (pieceId);

        if (piece == nullptr)
            continue;

        const auto subfolder = sampleSubfolderForPiece (piece->type);

        for (auto& art : piece->articulations)
        {
            for (auto& layer : art.layers)
            {
                for (auto& sample : layer.roundRobins.samples)
                {
                    if (! selfContained)
                    {
                        // Referenced mode: keep the sampleRef, do not copy audio.
                        continue;
                    }

                    const juce::File sourceFile (sample.filePath);

                    if (! sourceFile.existsAsFile())
                        continue;

                    const auto sourceKey = sourceFile.getFullPathName();

                    if (copiedSourcePaths.contains (sourceKey))
                    {
                        sample.filePath = copiedSourcePaths[sourceKey];
                        sample.sampleRef.relativePath = sample.filePath;
                        sample.sampleRef.sourceType = SampleRef::SourceType::embedded;
                        continue;
                    }

                    const auto destName = uniqueSampleFileName (subfolder, sourceFile, usedNames);
                    const auto destDir = outputFolder.getChildFile ("samples").getChildFile (subfolder);
                    destDir.createDirectory();
                    const auto destFile = destDir.getChildFile (destName);

                    if (! destFile.existsAsFile())
                    {
                        if (! sourceFile.copyFileTo (destFile))
                        {
                            result.errorMessage = "Failed to copy sample: " + sourceFile.getFullPathName();
                            return result;
                        }
                    }

                    const auto relativePath = "samples/" + subfolder + "/" + destName;
                    sample.filePath = relativePath;
                    sample.sampleRef.relativePath = relativePath;
                    sample.sampleRef.sourceType = SampleRef::SourceType::embedded;
                    copiedSourcePaths.set (sourceKey, relativePath);
                }
            }
        }
    }

    juce::String writeError;

    if (! KitPackageJsonSerializer::writeKitToFile (outputFolder.getChildFile ("kit.json"),
                                                     kitCopy,
                                                     packageId,
                                                     writeError))
    {
        result.errorMessage = writeError.isNotEmpty() ? writeError : "Failed to write kit.json.";
        return result;
    }

    const auto credits = options.creditsText.isNotEmpty()
                           ? options.creditsText
                           : defaultCreditsText (options.kitName);
    outputFolder.getChildFile ("credits.txt").replaceWithText (credits);

    if (options.licenseText.isNotEmpty())
        outputFolder.getChildFile ("license.txt").replaceWithText (options.licenseText);
    else if (options.generatePlaceholderLicense)
        outputFolder.getChildFile ("license.txt").replaceWithText (defaultLicensePlaceholder());

    if (options.artworkFile.existsAsFile())
        options.artworkFile.copyFileTo (outputFolder.getChildFile ("artwork").getChildFile ("thumbnail.png"));

    if (options.previewAudioFile.existsAsFile())
        options.previewAudioFile.copyFileTo (outputFolder.getChildFile ("preview").getChildFile ("preview.wav"));

    const auto now = juce::Time::getCurrentTime().toISO8601 (true);
    KitManifest manifest;
    manifest.packageId = packageId;
    manifest.name = options.kitName;
    manifest.version = options.version;
    manifest.author = options.author;
    manifest.createdAt = now;
    manifest.updatedAt = now;
    manifest.license = options.licenseType;
    manifest.tags = options.tags;
    manifest.sampleCount = KitPackageJsonSerializer::countSamplesInKit (kitCopy);
    manifest.pieceCount = (int) kitCopy.getPieces().size();
    manifest.installSizeBytes = outputFolder.getSize();
    manifest.referenceMode = selfContained ? "selfContained" : "referenced";

    for (const auto& usage : libraryUsage)
    {
        KitDependency dep;
        dep.libraryId = usage.first;
        dep.usedSampleCount = usage.second;
        dep.name = usage.first;

        for (const auto& known : options.dependencyCatalog)
        {
            if (known.libraryId == usage.first)
            {
                dep.name = known.name.isNotEmpty() ? known.name : usage.first;
                dep.license = known.license;
                break;
            }
        }

        manifest.dependencies.push_back (std::move (dep));
    }

    juce::String manifestError;

    if (! ManifestSerializer::writeToFile (outputFolder.getChildFile ("manifest.json"), manifest, manifestError))
    {
        result.errorMessage = manifestError.isNotEmpty() ? manifestError : "Failed to write manifest.json.";
        return result;
    }

    result.manifest = manifest;
    result.success = true;
    return result;
}

KitForgePackageWriter::WriteResult KitForgePackageWriter::writePackage (const KitModel& kit,
                                                                         const juce::File& packageFile,
                                                                         const WriteOptions& options) const
{
    const auto staging = packageFile.getSiblingFile (packageFile.getFileNameWithoutExtension() + "_staging");
    staging.deleteRecursively();

    auto result = writeFolder (kit, staging, options);

    if (! result.success)
        return result;

    juce::String zipError;

    if (! zipFolder (staging, packageFile, zipError))
    {
        result.errorMessage = "Package folder written but zip failed: " + zipError;
        result.success = false;
        staging.deleteRecursively();
        return result;
    }

    result.packageFile = packageFile;
    staging.deleteRecursively();
    return result;
}
