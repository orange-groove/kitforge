#include "KontaktImporter.h"
#include "KontaktSampleNameParser.h"
#include "KitImportLayoutEnforcer.h"
#include "../Engine/Articulation.h"
#include "../Engine/DrumSample.h"
#include "../Engine/SampleLayer.h"

namespace
{
    juce::String artGroupKey (const KontaktParsedSample& s)
    {
        return s.pieceKey + "|" + s.articulationName + "|" + juce::String (s.midiNote);
    }

    KontaktParseOptions parseOptionsFrom (const KontaktImportOptions& options)
    {
        KontaktParseOptions parseOpts;
        parseOpts.preferSnaresOnOnly = options.preferSnaresOnOnly;
        parseOpts.consolidateHiHatClosed = options.consolidateHiHatClosed;
        parseOpts.consolidateHiHatOpen = options.consolidateHiHatOpen;
        parseOpts.rideCrashAsCrashPiece = options.rideCrashAsCrashPiece;
        return parseOpts;
    }

    int preferredPrimaryMidi (const DrumPiece& piece)
    {
        static const char* preferredNames[] = { "Center", "Bow", "Closed", "Hit", "Edge" };

        for (const auto* name : preferredNames)
        {
            for (const auto& art : piece.articulations)
            {
                if (art.name.equalsIgnoreCase (name))
                    return art.midiNote;
            }
        }

        if (piece.name.containsIgnoreCase ("bop"))
            return 35;

        switch (piece.type)
        {
            case DrumPieceType::kick:     return 36;
            case DrumPieceType::snare:    return 38;
            case DrumPieceType::rackTom:  return 48;
            case DrumPieceType::floorTom: return 43;
            case DrumPieceType::hiHat:    return 42;
            case DrumPieceType::crash:    return 49;
            case DrumPieceType::ride:     return 51;
            default: break;
        }

        return piece.primaryMidiNote;
    }
}

juce::File KontaktImporter::findSamplesFolder (const juce::File& libraryRoot)
{
    const auto samples = libraryRoot.getChildFile ("samples");

    if (samples.isDirectory())
        return samples;

    if (libraryRoot.getNumberOfChildFiles (juce::File::findFiles, "*.wav") > 0)
        return libraryRoot;

    return {};
}

juce::String KontaktImporter::inferKitName (const juce::File& libraryRoot)
{
    for (const auto& f : libraryRoot.findChildFiles (juce::File::findFiles, false, "*.nki"))
        return f.getFileNameWithoutExtension();

    return libraryRoot.getFileName();
}

ImportResult KontaktImporter::importLibraryFolder (const juce::File& libraryRoot,
                                                    const KontaktImportOptions& options) const
{
    ImportResult result;
    result.importFormat = "kontakt";
    result.sourceRoot = libraryRoot;

    if (! libraryRoot.isDirectory())
    {
        result.errorMessage = "Kontakt library folder not found.";
        return result;
    }

    const auto samplesDir = findSamplesFolder (libraryRoot);

    if (! samplesDir.isDirectory())
    {
        result.errorMessage = "No samples/ folder or WAV files found in Kontakt library.";
        return result;
    }

    result.kitName = inferKitName (libraryRoot);

    KontaktSampleNameParser parser (parseOptionsFrom (options));
    std::vector<KontaktParsedSample> parsed;
    int wavCount = 0;
    int skipped = 0;

    for (const auto& file : samplesDir.findChildFiles (juce::File::findFiles, true, "*.wav"))
    {
        ++wavCount;
        const auto entry = parser.parseFile (file);

        if (! entry.has_value())
        {
            result.warnings.push_back (ImportWarning::make (ImportWarningType::unknownSample,
                                                            "Unrecognized Kontakt filename: "
                                                                + file.getFileName(),
                                                            file.getFullPathName()));
            continue;
        }

        if (entry->skipped)
        {
            ++skipped;
            continue;
        }

        parsed.push_back (*entry);
    }

    if (parsed.empty())
    {
        result.errorMessage = "No recognizable Kontakt drum samples found.";
        return result;
    }

    juce::StringArray groupKeys;
    juce::HashMap<juce::String, juce::Array<int>> indexGroups;

    for (int i = 0; i < (int) parsed.size(); ++i)
    {
        const auto key = artGroupKey (parsed[(size_t) i]);

        if (! indexGroups.contains (key))
            groupKeys.add (key);

        indexGroups.getReference (key).add (i);
    }

    for (const auto& key : groupKeys)
    {
        auto& indices = indexGroups.getReference (key);
        std::vector<KontaktParsedSample> group;

        for (const auto idx : indices)
            group.push_back (parsed[(size_t) idx]);

        KontaktSampleNameParser::assignVelocityLayers (group);

        for (int j = 0; j < indices.size(); ++j)
            parsed[(size_t) indices[j]] = group[(size_t) j];
    }

    if (skipped > 0)
    {
        result.warnings.push_back (ImportWarning::make (ImportWarningType::general,
                                                        "Skipped " + juce::String (skipped)
                                                            + " sample(s) (utility switches and snares-off variants)."));
    }

    result.warnings.push_back (ImportWarning::make (ImportWarningType::general,
                                                    "Imported from Kontakt sample naming (.nki mapping not parsed)."));

    result.kit = buildKitFromSamples (result.kitName, parsed, result.warnings);
    fixPrimaryMidiNotes (result.kit);
    applyImportKitLayout (result.kit);
    result.stats = ImportResult::computeStats (result.kit, wavCount - skipped);
    result.success = true;
    return result;
}

KitModel KontaktImporter::buildKitFromSamples (const juce::String& kitName,
                                                const std::vector<KontaktParsedSample>& samples,
                                                std::vector<ImportWarning>& warnings)
{
    juce::ignoreUnused (warnings);

    KitModel model = KitModel::createEmpty();
    model.kitName = kitName;

    juce::HashMap<juce::String, DrumPiece*> piecesByKey;

    for (const auto& meta : samples)
    {
        DrumPiece* piece = piecesByKey[meta.pieceKey];

        if (piece == nullptr)
        {
            DrumPiece newPiece;
            newPiece.id = DrumPiece::makeId();
            newPiece.name = meta.pieceName;
            newPiece.type = meta.pieceType;
            newPiece.primaryMidiNote = meta.midiNote;
            newPiece.width = defaultPieceSize (meta.pieceType);
            newPiece.height = defaultPieceSize (meta.pieceType);
            normalizePieceVisuals (newPiece);

            if (meta.pieceType == DrumPieceType::hiHat)
                newPiece.chokeGroupId = "hihat";

            auto& added = model.addPiece (std::move (newPiece));
            piece = &added;
            piecesByKey.set (meta.pieceKey, piece);
        }

        Articulation* art = nullptr;

        for (auto& existing : piece->articulations)
        {
            if (existing.name == meta.articulationName && existing.midiNote == meta.midiNote)
            {
                art = &existing;
                break;
            }
        }

        if (art == nullptr)
        {
            Articulation newArt;
            newArt.id = Articulation::makeId();
            newArt.name = meta.articulationName;
            newArt.midiNote = meta.midiNote;
            newArt.chokeGroupId = meta.chokeGroupId;
            art = &piece->addArticulation (std::move (newArt));
        }

        SampleLayer* layer = nullptr;

        for (auto& existing : art->layers)
        {
            if (existing.minVelocity == meta.minVelocity && existing.maxVelocity == meta.maxVelocity)
            {
                layer = &existing;
                break;
            }
        }

        if (layer == nullptr)
        {
            SampleLayer newLayer;
            newLayer.id = SampleLayer::makeId();
            newLayer.minVelocity = meta.minVelocity;
            newLayer.maxVelocity = meta.maxVelocity;
            art->layers.push_back (std::move (newLayer));
            layer = &art->layers.back();
        }

        DrumSample sample;
        sample.id = DrumSample::makeId();
        sample.filePath = meta.file.getFullPathName();
        sample.rootMidiNote = meta.midiNote;

        if (meta.roundRobinIndex > 0)
        {
            auto& rr = layer->roundRobins.samples;
            const int insertAt = juce::jmax (0, meta.roundRobinIndex - 1);

            if (insertAt >= (int) rr.size())
                rr.push_back (std::move (sample));
            else
                rr.insert (rr.begin() + insertAt, std::move (sample));
        }
        else
        {
            layer->roundRobins.addSample (std::move (sample));
        }

        piece->syncMidiNotesFromArticulations();
    }

    return model;
}

void KontaktImporter::fixPrimaryMidiNotes (KitModel& model)
{
    juce::StringArray pieceIds;

    for (const auto& piece : model.getPieces())
        pieceIds.add (piece.id);

    for (const auto& pieceId : pieceIds)
    {
        if (auto* piece = model.findPieceById (pieceId))
        {
            piece->primaryMidiNote = preferredPrimaryMidi (*piece);
            piece->syncMidiNotesFromArticulations();
            piece->primaryMidiNote = preferredPrimaryMidi (*piece);
        }
    }
}
