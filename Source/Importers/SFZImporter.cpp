#include "SFZImporter.h"
#include "KitModelBuilder.h"
#include "SampleNameParser.h"

namespace
{
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

    const auto regions = parseRegions (sfzFile.loadFileAsString());

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

    result.kit = KitModelBuilder::buildFromMetadata (result.kitName, samples, result.warnings);
    result.stats = ImportResult::computeStats (result.kit, wavCount);
    result.success = true;
    return result;
}

std::vector<SFZImporter::SFZRegion> SFZImporter::parseRegions (const juce::String& sfzText) const
{
    std::vector<SFZRegion> regions;
    SFZRegion current;
    SFZRegion groupDefaults;
    SFZRegion masterDefaults;

    enum class Block { none, master, group, region };
    Block block = Block::none;

    for (const auto& line : juce::StringArray::fromLines (sfzText))
    {
        const auto trimmed = line.trim();

        if (trimmed.startsWith ("<master>"))
        {
            masterDefaults = SFZRegion();
            groupDefaults = SFZRegion();
            block = Block::master;
            continue;
        }

        if (trimmed.startsWith ("<group>"))
        {
            groupDefaults = masterDefaults;
            block = Block::group;
            continue;
        }

        if (trimmed.startsWith ("<region>"))
        {
            if (block == Block::region && current.samplePath.isNotEmpty())
                regions.push_back (current);

            current = groupDefaults;
            block = Block::region;
            continue;
        }

        if (trimmed.startsWithChar ('<'))
            continue;

        if (trimmed.isEmpty())
            continue;

        switch (block)
        {
            case Block::master: applyOpcodeLine (trimmed, masterDefaults); break;
            case Block::group:  applyOpcodeLine (trimmed, groupDefaults); break;
            case Block::region: applyOpcodeLine (trimmed, current); break;
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
        case 48: return DrumPieceType::rackTom;
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
    const juce::File sampleFile = sampleRoot.getChildFile (region.samplePath);
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
        const auto parsed = parser.parseFile (sampleFile);
        midiNote = parsed.midiNote;
        meta.instrumentType = parsed.instrumentType;
        meta.articulation = parsed.articulation;
        meta.instrumentIndex = parsed.instrumentIndex;
        meta.confidence = parsed.confidence;
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

    if (meta.instrumentType == DrumPieceType::accessory)
        meta.instrumentType = inferTypeFromMidiNote (midiNote);

    if (meta.articulation.isEmpty())
    {
        meta.articulation = inferArticulationFromSamplePath (meta.filePath);

        if (meta.articulation.isEmpty())
            meta.articulation = inferArticulationFromMidiNote (midiNote, meta.instrumentType);
    }

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

    if (meta.instrumentType == DrumPieceType::crash)
        meta.instrumentIndex = midiNote == 57 ? 1 : 0;
    else if (meta.instrumentType == DrumPieceType::rackTom)
    {
        if (midiNote == 47) meta.instrumentIndex = 1;
        else if (midiNote == 45) meta.instrumentIndex = 2;
    }
    else if (meta.instrumentType == DrumPieceType::floorTom)
        meta.instrumentIndex = midiNote == 41 ? 1 : 0;

    if (meta.confidence <= 0.0f)
        meta.confidence = 0.7f;

    meta.tags.add (drumPieceTypeToString (meta.instrumentType));
    meta.tags.add (meta.articulation.toLowerCase());

    juce::ignoreUnused (warnings);
    return meta;
}
