#include "SFZImporter.h"
#include "KitModelBuilder.h"
#include "KitImportLayoutEnforcer.h"
#include "SampleNameParser.h"
#include "SfzDrumMappingResolver.h"
#include "LlmSampleClassifier.h"
#include <functional>

namespace
{
    juce::File resolveRelativeSamplePath (const juce::File& root, juce::String relativePath)
    {
        relativePath = relativePath.replaceCharacter ('\\', '/');
        juce::File result = root;

        for (const auto& part : juce::StringArray::fromTokens (relativePath, "/", ""))
        {
            if (part == "..")
                result = result.getParentDirectory();
            else if (part.isNotEmpty() && part != ".")
                result = result.getChildFile (part);
        }

        return result;
    }

    int readOpcodeInt (const juce::String& line, const juce::String& opcode, int fallback)
    {
        if (line.trimStart().startsWithIgnoreCase (opcode))
        {
            const auto value = line.fromFirstOccurrenceOf ("=", false, false).trim();

            if (value.isNotEmpty())
                return value.getIntValue();
        }

        return fallback;
    }

    float readOpcodeFloat (const juce::String& line, const juce::String& opcode, float fallback)
    {
        if (line.trimStart().startsWithIgnoreCase (opcode))
        {
            const auto value = line.fromFirstOccurrenceOf ("=", false, false).trim();

            if (value.isNotEmpty())
                return (float) value.getDoubleValue();
        }

        return fallback;
    }

    juce::String readOpcodeString (const juce::String& line, const juce::String& opcode)
    {
        if (line.trimStart().startsWithIgnoreCase (opcode))
            return line.fromFirstOccurrenceOf ("=", false, false).trim();

        return {};
    }
}

ImportResult SFZImporter::importFile (const SFZImportOptions& options) const
{
    ImportResult result;
    result.importFormat = "sfz";
    result.sourceRoot = juce::File (options.sampleRootPath);

    const juce::File sfzFile (options.sfzFilePath);

    if (! sfzFile.existsAsFile())
    {
        result.errorMessage = "SFZ file not found: " + options.sfzFilePath;
        return result;
    }

    result.sourceRoot = options.sampleRootPath.isNotEmpty()
                          ? juce::File (options.sampleRootPath)
                          : sfzFile.getParentDirectory();

    const auto expandedSfz = expandSfzDocument (sfzFile);

    if (expandedSfz.isEmpty())
    {
        result.errorMessage = "Could not read or expand SFZ file: " + options.sfzFilePath;
        return result;
    }

    const auto regions = parseRegions (expandedSfz);

    if (regions.empty())
    {
        result.errorMessage = "No regions found in SFZ file.";
        return result;
    }

    result.kitName = options.kitName.isNotEmpty() ? options.kitName : sfzFile.getFileNameWithoutExtension();

    std::vector<SampleMetadata> samples;
    int wavCount = 0;

    for (const auto& region : regions)
    {
        auto meta = regionToMetadata (region, options, result.warnings);

        if (meta.filePath.isEmpty())
            continue;

        if (juce::File (meta.filePath).hasFileExtension ("wav"))
            ++wavCount;

        samples.push_back (std::move (meta));
    }

    if (samples.empty())
    {
        result.errorMessage = "No valid sample regions found in SFZ file.";
        return result;
    }

    // Prefer LLM grouping (robust to vendor naming); silently keeps the
    // resolver-derived values when unavailable/offline/invalid.
    LlmSampleClassifier::classify (result.kitName, samples);

    result.kit = KitModelBuilder::buildFromMetadata (result.kitName, samples, result.warnings);
    applyImportKitLayout (result.kit, 980.0f, 680.0f);
    result.stats = ImportResult::computeStats (result.kit, wavCount);
    result.success = true;
    return result;
}

juce::String SFZImporter::stripInlineComment (const juce::String& line)
{
    return line.upToFirstOccurrenceOf ("//", false, false).trim();
}

void SFZImporter::applyDefineSubstitutions (juce::String& line,
                                              const juce::HashMap<juce::String, juce::String>& defines) const
{
    for (auto it = defines.begin(); it != defines.end(); ++it)
        line = line.replace (it.getKey(), it.getValue());
}

juce::String SFZImporter::expandSfzDocument (const juce::File& sfzFile) const
{
    if (! sfzFile.existsAsFile())
        return {};

    juce::HashMap<juce::String, juce::String> defines;
    juce::StringArray visitedPaths;
    juce::String output;

    std::function<void(const juce::File&)> expandFile;

    expandFile = [&] (const juce::File& file)
    {
        const auto canonical = file.getFullPathName();

        if (visitedPaths.contains (canonical, true))
            return;

        visitedPaths.add (canonical);

        if (! file.existsAsFile())
            return;

        const auto masterRoot = sfzFile.getParentDirectory();

        for (const auto& line : juce::StringArray::fromLines (file.loadFileAsString()))
        {
            auto trimmed = stripInlineComment (line.trim());

            if (trimmed.isEmpty())
                continue;

            if (trimmed.startsWithIgnoreCase ("#define"))
            {
                const auto body = trimmed.substring (7).trim();

                if (body.isNotEmpty())
                {
                    const auto space = body.indexOfAnyOf (" \t");

                    if (space > 0)
                    {
                        const auto name = body.substring (0, space).trim();
                        const auto value = body.substring (space + 1).trim();
                        defines.set (name, value);
                    }
                }

                continue;
            }

            if (trimmed.startsWithIgnoreCase ("#include"))
            {
                const int q1 = trimmed.indexOfChar ('"');

                if (q1 >= 0)
                {
                    const int q2 = trimmed.indexOfChar (q1 + 1, '"');

                    if (q2 > q1)
                    {
                        const auto includePath = trimmed.substring (q1 + 1, q2);
                        output << "// kitforge:map=" << includePath.replaceCharacter ('\\', '/') << "\n";
                        expandFile (file.getSiblingFile (includePath));
                    }
                }

                continue;
            }

            if (trimmed.startsWithChar ('#') && ! trimmed.startsWithChar ('<'))
                continue;

            auto expandedLine = trimmed;
            applyDefineSubstitutions (expandedLine, defines);

            if (expandedLine.trimStart().startsWithIgnoreCase ("sample"))
            {
                const auto samplePath = readOpcodeString (expandedLine, "sample");

                if (samplePath.isNotEmpty())
                {
                    // Multi-file SFZ libraries (e.g. Sforzando) resolve sample paths from the
                    // master instrument folder, not the included mapping file's folder.
                    const auto absolute = resolveRelativeSamplePath (masterRoot, samplePath);
                    auto relativeToMaster = absolute.getRelativePathFrom (masterRoot).replaceCharacter ('\\', '/');
                    expandedLine = "sample=" + relativeToMaster;
                }
            }

            output << expandedLine << "\n";
        }
    };

    expandFile (sfzFile);
    return output;
}

juce::String SFZImporter::stripSfzTagSuffix (const juce::String& line)
{
    const auto trimmed = line.trim();

    if (! trimmed.startsWithChar ('<'))
        return {};

    const int close = trimmed.indexOfChar ('>');

    if (close < 0)
        return {};

    return stripInlineComment (trimmed.substring (close + 1));
}

void SFZImporter::applyOpcodeTokens (const juce::String& line, SFZRegion& region) const
{
    const auto cleaned = stripInlineComment (line);

    // Sample paths often contain spaces; never split sample= lines on whitespace.
    if (cleaned.trimStart().startsWithIgnoreCase ("sample="))
    {
        applyOpcodeLine (cleaned, region);
        return;
    }

    for (const auto& token : juce::StringArray::fromTokens (cleaned, " ", ""))
    {
        if (token.containsChar ('='))
            applyOpcodeLine (token, region);
    }
}

std::vector<SFZImporter::SFZRegion> SFZImporter::parseRegions (const juce::String& sfzText) const
{
    std::vector<SFZRegion> regions;
    SFZRegion current;
    SFZRegion groupDefaults;
    SFZRegion masterDefaults;
    juce::String currentMappingSource;

    enum class Block { none, master, group, region };
    Block block = Block::none;

    for (const auto& line : juce::StringArray::fromLines (sfzText))
    {
        const auto trimmed = stripInlineComment (line.trim());

        if (trimmed.startsWith ("// kitforge:map="))
        {
            currentMappingSource = trimmed.fromFirstOccurrenceOf ("map=", false, false).trim();
            continue;
        }

        if (trimmed.startsWith ("<master>") || trimmed.startsWith ("<global>"))
        {
            masterDefaults = SFZRegion();
            groupDefaults = SFZRegion();
            block = Block::master;
            applyOpcodeTokens (stripSfzTagSuffix (trimmed), masterDefaults);
            continue;
        }

        if (trimmed.startsWith ("<group>"))
        {
            groupDefaults = masterDefaults;
            block = Block::group;
            applyOpcodeTokens (stripSfzTagSuffix (trimmed), groupDefaults);
            continue;
        }

        if (trimmed.startsWith ("<region>"))
        {
            if (block == Block::region && current.samplePath.isNotEmpty())
                regions.push_back (current);

            current = groupDefaults;
            current.mappingSource = currentMappingSource;
            block = Block::region;
            applyOpcodeTokens (stripSfzTagSuffix (trimmed), current);
            continue;
        }

        if (trimmed.startsWithChar ('<'))
            continue;

        if (trimmed.isEmpty())
            continue;

        switch (block)
        {
            case Block::master: applyOpcodeTokens (trimmed, masterDefaults); break;
            case Block::group:  applyOpcodeTokens (trimmed, groupDefaults); break;
            case Block::region: applyOpcodeTokens (trimmed, current); break;
            default: break;
        }
    }

    if (block == Block::region && current.samplePath.isNotEmpty())
        regions.push_back (current);

    return regions;
}

void SFZImporter::applyOpcodeLine (const juce::String& line, SFZRegion& region) const
{
    if (const auto key = readOpcodeInt (line, "key", -999); key != -999)
        region.key = key;
    else if (const auto lokey = readOpcodeInt (line, "lokey", -999); lokey != -999)
        region.lokey = lokey;
    else if (const auto hikey = readOpcodeInt (line, "hikey", -999); hikey != -999)
        region.hikey = hikey;
    else if (const auto pkc = readOpcodeInt (line, "pitch_keycenter", -999); pkc != -999)
        region.pitchKeyCenter = pkc;
    else if (const auto lovel = readOpcodeInt (line, "lovel", -999); lovel != -999)
        region.lovel = lovel;
    else if (const auto hivel = readOpcodeInt (line, "hivel", -999); hivel != -999)
        region.hivel = hivel;
    else if (const auto group = readOpcodeInt (line, "group", -999); group != -999)
        region.group = group;
    else if (const auto offBy = readOpcodeInt (line, "off_by", -999); offBy != -999)
        region.offBy = offBy;
    else if (const auto seqLength = readOpcodeInt (line, "seq_length", -999); seqLength != -999)
        region.seqLength = seqLength;
    else if (const auto seqPos = readOpcodeInt (line, "seq_position", -999); seqPos != -999)
        region.seqPosition = seqPos;
    else if (const auto loRand = readOpcodeFloat (line, "lorand", -999.0f); loRand != -999.0f)
        region.loRand = loRand;
    else if (const auto hiRand = readOpcodeFloat (line, "hirand", -999.0f); hiRand != -999.0f)
        region.hiRand = hiRand;
    else if (const auto sample = readOpcodeString (line, "sample"); sample.isNotEmpty())
        region.samplePath = sample;
}

DrumPieceType SFZImporter::inferTypeFromMidiNote (int midiNote)
{
    switch (midiNote)
    {
        case 35:
        case 36: return DrumPieceType::kick;
        case 37:
        case 38:
        case 40: return DrumPieceType::snare;
        case 41:
        case 43: return DrumPieceType::floorTom;
        case 42:
        case 44:
        case 46: return DrumPieceType::hiHat;
        case 45:
        case 47:
        case 48:
        case 50: return DrumPieceType::rackTom;
        case 49:
        case 57: return DrumPieceType::crash;
        case 51:
        case 53:
        case 59: return DrumPieceType::ride;
        case 52: return DrumPieceType::china;
        case 55: return DrumPieceType::splash;
        default: return DrumPieceType::accessory;
    }
}

int SFZImporter::inferIndexFromMidiNote (int midiNote, DrumPieceType type)
{
    switch (type)
    {
        case DrumPieceType::kick:
            return midiNote == 35 ? 1 : 0;

        case DrumPieceType::rackTom:
            switch (midiNote)
            {
                case 45: return 0;
                case 47: return 1;
                case 48: return 2;
                case 50: return 3;
                default: return 0;
            }

        case DrumPieceType::floorTom:
            return midiNote == 41 ? 1 : 0;

        case DrumPieceType::crash:
            return midiNote == 57 ? 1 : 0;

        default:
            return 0;
    }
}

juce::String SFZImporter::inferArticulationFromMidiNote (int midiNote, DrumPieceType type)
{
    switch (midiNote)
    {
        case 37: return "Sidestick";
        case 40: return "Rimshot";
        case 42: return "Closed";
        case 44: return "Pedal";
        case 46: return "Open";
        case 53: return "Bell";
        case 59: return "Edge";
        default: break;
    }

    if (type == DrumPieceType::kick)
        return "Center";

    if (type == DrumPieceType::ride)
        return "Bow";

    return "Center";
}

juce::String SFZImporter::inferArticulationFromSamplePath (const juce::String& samplePath)
{
    SampleNameParser parser;
    return parser.parseFile (juce::File (samplePath)).articulation;
}

SampleMetadata SFZImporter::regionToMetadata (const SFZRegion& region,
                                               const SFZImportOptions& options,
                                               std::vector<ImportWarning>& warnings) const
{
    SampleMetadata meta;
    meta.id = juce::Uuid().toString();

    if (region.samplePath.isEmpty())
        return meta;

    const juce::File sampleRoot (options.sampleRootPath.isNotEmpty()
                                     ? options.sampleRootPath
                                     : juce::File (options.sfzFilePath).getParentDirectory().getFullPathName());
    const juce::File sampleFile = resolveRelativeSamplePath (sampleRoot, region.samplePath);
    meta.filePath = sampleFile.getFullPathName();

    if (! sampleFile.existsAsFile())
    {
        warnings.push_back (ImportWarning::make (ImportWarningType::sampleFileMissing,
                                                 "Sample file missing from SFZ: " + region.samplePath,
                                                 meta.filePath));
    }

    int midiNote = region.key;

    if (midiNote < 0)
        midiNote = region.pitchKeyCenter;

    if (midiNote < 0)
        midiNote = region.lokey;

    if (midiNote < 0)
    {
        SampleNameParser parser;
        midiNote = parser.parseFile (sampleFile).midiNote;
    }

    if (midiNote < 0)
    {
        warnings.push_back (ImportWarning::make (ImportWarningType::couldNotInferInstrument,
                                                 "Could not determine MIDI note for: " + region.samplePath,
                                                 meta.filePath));
        midiNote = 36;
        meta.confidence = 0.0f;
    }

    meta.midiNote = midiNote;
    meta.minVelocity = juce::jlimit (1, 127, region.lovel);
    meta.maxVelocity = juce::jlimit (1, 127, region.hivel);
    meta.velocityValue = (meta.minVelocity + meta.maxVelocity) / 2;

    auto hint = SfzDrumMappingResolver::fromMappingSource (region.mappingSource);

    if (hint.confidence <= 0.0f)
        hint = SfzDrumMappingResolver::fromSamplePath (sampleFile);

    if (hint.confidence <= 0.0f)
    {
        hint.type = inferTypeFromMidiNote (midiNote);
        hint.index = inferIndexFromMidiNote (midiNote, hint.type);
        hint.articulation = inferArticulationFromMidiNote (midiNote, hint.type);
        hint.confidence = 0.5f;
    }

    meta.instrumentType = hint.type;
    meta.instrumentIndex = hint.index;
    meta.articulation = hint.articulation;
    meta.confidence = hint.confidence;

    if (region.seqPosition > 0)
        meta.roundRobinIndex = region.seqPosition;
    else if (region.seqLength > 1)
        meta.roundRobinIndex = 1;
    else if (region.hiRand < 1.0f)
        meta.roundRobinIndex = (int) (region.loRand * 100.0f) + 1;

    if (meta.instrumentType == DrumPieceType::hiHat
        || meta.instrumentType == DrumPieceType::crash
        || meta.instrumentType == DrumPieceType::ride)
    {
        if (region.offBy != 0 || region.group != 0)
            meta.chokeGroupId = meta.instrumentType == DrumPieceType::hiHat
                                    ? "hihat"
                                    : "cymbal_" + juce::String (region.group);
    }

    meta.tags.add (drumPieceTypeToString (meta.instrumentType));
    meta.tags.add (meta.articulation.toLowerCase());

    juce::ignoreUnused (warnings);
    return meta;
}
