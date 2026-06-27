#include "SampleNameParser.h"

namespace
{
    juce::String normalizeToken (const juce::String& s)
    {
        return s.trim().toLowerCase().removeCharacters (" -");
    }

    int extractTrailingNumber (const juce::String& token)
    {
        juce::String digits;

        for (int i = token.length() - 1; i >= 0; --i)
        {
            const auto c = token[i];

            if (juce::CharacterFunctions::isDigit (c))
                digits = juce::String::charToString (c) + digits;
            else
                break;
        }

        return digits.isEmpty() ? 0 : digits.getIntValue();
    }

    juce::String stripTrailingNumber (const juce::String& token)
    {
        int end = token.length();

        while (end > 0 && juce::CharacterFunctions::isDigit (token[end - 1]))
            --end;

        return token.substring (0, end);
    }
}

SampleMetadata SampleNameParser::parseFile (const juce::File& file) const
{
    SampleMetadata meta;
    meta.id = juce::Uuid().toString();
    meta.filePath = file.getFullPathName();

    if (! file.hasFileExtension ("wav"))
        return meta;

    const auto baseName = file.getFileNameWithoutExtension();
    const auto tokens = juce::StringArray::fromTokens (baseName, "_-", "");

    InstrumentMatch bestMatch;
    bestMatch.confidence = 0.0f;
    juce::String articulationCandidate;
    bool hasStructuredName = false;

    for (const auto& rawToken : tokens)
    {
        const auto token = normalizeToken (rawToken);

        if (token.isEmpty())
            continue;

        if (isVelocityToken (token))
        {
            applyVelocityToken (token, meta);
            hasStructuredName = true;
            continue;
        }

        if (isRoundRobinToken (token))
        {
            applyRoundRobinToken (token, meta);
            hasStructuredName = true;
            continue;
        }

        auto instrumentMatch = matchInstrumentToken (token);

        if (instrumentMatch.confidence <= bestMatch.confidence)
            instrumentMatch = matchCompoundInstrumentToken (token);

        if (instrumentMatch.confidence > bestMatch.confidence)
        {
            bestMatch = instrumentMatch;

            if (instrumentMatch.articulation.isNotEmpty())
                articulationCandidate = instrumentMatch.articulation;
            else
                articulationCandidate.clear();

            hasStructuredName = true;
        }
        else if (bestMatch.type != DrumPieceType::accessory)
        {
            const auto artMatch = matchArticulationToken (token, bestMatch.type);

            if (artMatch.confidence > 0.0f)
            {
                if (artMatch.articulation.isNotEmpty())
                    articulationCandidate = artMatch.articulation;

                if (artMatch.midiNote > 0)
                    bestMatch.midiNote = artMatch.midiNote;

                if (artMatch.chokeGroupId.isNotEmpty())
                    bestMatch.chokeGroupId = artMatch.chokeGroupId;

                hasStructuredName = true;
            }
            else if (articulationCandidate.isEmpty()
                     && token != "hit" && token != "sample" && token != "wav")
            {
                articulationCandidate = rawToken.trim();
                hasStructuredName = true;
            }
        }
    }

    float folderConfidence = 0.0f;
    auto folder = file.getParentDirectory();

    for (int depth = 0; depth < 6 && folder.exists(); ++depth)
    {
        const auto folderMatch = inferFromFolderName (folder.getFileName());

        if (folderMatch.confidence > folderConfidence)
        {
            folderConfidence = folderMatch.confidence;

            if (bestMatch.confidence < folderMatch.confidence)
            {
                bestMatch.type = folderMatch.type;
                bestMatch.index = folderMatch.index;
            }
        }

        folder = folder.getParentDirectory();
    }

    meta.instrumentType = bestMatch.type;
    meta.instrumentIndex = juce::jmax (0, bestMatch.index > 0 ? bestMatch.index - 1 : bestMatch.index);
    meta.chokeGroupId = bestMatch.chokeGroupId;

    if (articulationCandidate.isNotEmpty())
        meta.articulation = articulationCandidate;
    else
        meta.articulation = defaultArticulationForType (meta.instrumentType);

    if (bestMatch.midiNote > 0)
        meta.midiNote = bestMatch.midiNote;
    else
        meta.midiNote = defaultMidiForTypeAndArticulation (meta.instrumentType,
                                                           meta.instrumentIndex,
                                                           meta.articulation);

    if (hasStructuredName && bestMatch.confidence >= 0.7f)
        meta.confidence = 1.0f;
    else if (hasStructuredName || folderConfidence >= 0.7f)
        meta.confidence = 0.7f;
    else if (bestMatch.type != DrumPieceType::accessory || folderConfidence > 0.0f)
        meta.confidence = 0.4f;
    else
        meta.confidence = 0.0f;

    finalizeMetadata (meta);
    return meta;
}

SampleNameParser::InstrumentMatch SampleNameParser::matchInstrumentToken (const juce::String& token) const
{
    InstrumentMatch match;
    const auto base = stripTrailingNumber (token);
    const int trailingNum = extractTrailingNumber (token);

    if (base == "kick" || base == "kicks" || base == "bd" || base == "bassdrum")
    {
        match.type = DrumPieceType::kick;
        match.confidence = 1.0f;
        match.midiNote = 36;
        return match;
    }

    if (base == "snare" || base == "snares" || base == "sd")
    {
        match.type = DrumPieceType::snare;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "racktom" || base == "racktoms" || base == "tom" || base == "toms"
        || base == "hightom" || base == "midtoms" || base == "midtom")
    {
        match.type = DrumPieceType::rackTom;
        match.index = trailingNum;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "floortom" || base == "floortoms" || base == "lowtom" || base == "floort")
    {
        match.type = DrumPieceType::floorTom;
        match.index = trailingNum;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "hihat" || base == "hat" || base == "hats" || base == "hh")
    {
        match.type = DrumPieceType::hiHat;
        match.chokeGroupId = "hihat";
        match.confidence = 1.0f;
        return match;
    }

    if (base == "crash" || base == "crashcym" || base == "crashcymbal")
    {
        match.type = DrumPieceType::crash;
        match.index = trailingNum > 0 ? trailingNum : 1;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "ride" || base == "ridecym" || base == "ridecymbal")
    {
        match.type = DrumPieceType::ride;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "china" || base == "chinese" || base == "chinesecym")
    {
        match.type = DrumPieceType::china;
        match.midiNote = 52;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "splash")
    {
        match.type = DrumPieceType::splash;
        match.midiNote = 55;
        match.confidence = 1.0f;
        return match;
    }

    if (base == "perc" || base == "percussion" || base == "aux" || base == "accessory")
    {
        match.type = DrumPieceType::accessory;
        match.confidence = 0.7f;
        return match;
    }

    return match;
}

SampleNameParser::InstrumentMatch SampleNameParser::matchCompoundInstrumentToken (const juce::String& token) const
{
    InstrumentMatch match;
    const auto norm = normalizeToken (token);

    struct SuffixArt
    {
        const char* suffix;
        const char* articulation;
    };

    static constexpr SuffixArt kSuffixArts[] = {
        { "bell", "Bell" },
        { "bow", "Bow" },
        { "edge", "Edge" },
        { "closed", "Closed" },
        { "open", "Open" },
        { "pedal", "Pedal" },
        { "rimshot", "Rimshot" },
        { "sidestick", "Sidestick" },
    };

    for (const auto& entry : kSuffixArts)
    {
        const juce::String suffix (entry.suffix);

        if (! norm.endsWith (suffix) || norm.length() <= (size_t) suffix.length())
            continue;

        const auto prefix = norm.substring (0, norm.length() - suffix.length());
        auto inst = matchInstrumentToken (prefix);

        if (inst.confidence <= 0.0f)
            continue;

        match = inst;
        match.articulation = entry.articulation;

        const auto artMatch = matchArticulationToken (suffix, inst.type);

        if (artMatch.midiNote > 0)
            match.midiNote = artMatch.midiNote;

        if (artMatch.chokeGroupId.isNotEmpty())
            match.chokeGroupId = artMatch.chokeGroupId;

        return match;
    }

    return match;
}

SampleNameParser::InstrumentMatch SampleNameParser::matchArticulationToken (const juce::String& token,
                                                                             DrumPieceType type) const
{
    InstrumentMatch match;

    if (token == "center" || token == "head" || token == "main" || token == "hit")
    {
        match.articulation = "Center";
        match.confidence = 1.0f;

        if (type == DrumPieceType::snare)
            match.midiNote = 38;

        return match;
    }

    if (token == "rim" || token == "rimshot" || token == "rimsh")
    {
        match.articulation = "Rimshot";
        match.midiNote = 40;
        match.confidence = 1.0f;
        return match;
    }

    if (token == "sidestick" || token == "side" || token == "crossstick" || token == "cross"
        || token == "stick")
    {
        match.articulation = "Sidestick";
        match.midiNote = 37;
        match.confidence = 1.0f;
        return match;
    }

    if (token == "closed" || token == "close" || token == "cl")
    {
        match.articulation = "Closed";
        match.midiNote = 42;
        match.chokeGroupId = "hihat";
        match.confidence = 1.0f;
        return match;
    }

    if (token == "open" || token == "op")
    {
        match.articulation = "Open";
        match.midiNote = 46;
        match.chokeGroupId = "hihat";
        match.confidence = 1.0f;
        return match;
    }

    if (token == "pedal" || token == "foot" || token == "chick" || token == "splashchick")
    {
        match.articulation = "Pedal";
        match.midiNote = 44;
        match.chokeGroupId = "hihat";
        match.confidence = 1.0f;
        return match;
    }

    if (token == "bow" || token == "middle" || token == "body")
    {
        match.articulation = "Bow";
        match.midiNote = 51;
        match.confidence = 1.0f;
        return match;
    }

    if (token == "bell")
    {
        match.articulation = "Bell";
        match.midiNote = 53;
        match.confidence = 1.0f;
        return match;
    }

    if (token == "edge" || token == "shoulder" || token == "shank")
    {
        match.articulation = "Edge";
        match.confidence = 1.0f;

        if (type == DrumPieceType::ride)
            match.midiNote = 59;
        else if (type == DrumPieceType::crash)
            match.midiNote = 0;

        return match;
    }

    return match;
}

SampleNameParser::InstrumentMatch SampleNameParser::inferFromFolderName (const juce::String& folderName) const
{
    return matchInstrumentToken (normalizeToken (folderName));
}

bool SampleNameParser::isVelocityToken (const juce::String& token) const
{
    if (token.startsWith ("v") && token.length() > 1)
    {
        const auto digits = token.substring (1);

        if (digits.containsOnly ("0123456789"))
            return true;
    }

    return token == "soft" || token == "medium" || token == "mid"
        || token == "hard" || token == "loud";
}

bool SampleNameParser::isRoundRobinToken (const juce::String& token) const
{
    if (token.startsWith ("rr") && token.length() > 2)
        return token.substring (2).containsOnly ("0123456789");

    if (token.startsWith ("roundrobin") && token.length() > 10)
        return token.substring (10).containsOnly ("0123456789");

    if (token.startsWith ("alt") && token.length() > 3)
        return token.substring (3).containsOnly ("0123456789");

    return false;
}

void SampleNameParser::applyVelocityToken (const juce::String& token, SampleMetadata& meta) const
{
    if (token.startsWith ("v") && token.length() > 1)
    {
        const int vel = token.substring (1).getIntValue();
        meta.velocityValue = juce::jlimit (1, 127, vel);
        meta.minVelocity = juce::jmax (1, vel - 3);
        meta.maxVelocity = juce::jmin (127, vel + 3);
        return;
    }

    if (token == "soft")
    {
        meta.minVelocity = 1;
        meta.maxVelocity = 45;
        meta.velocityValue = 30;
    }
    else if (token == "medium" || token == "mid")
    {
        meta.minVelocity = 46;
        meta.maxVelocity = 90;
        meta.velocityValue = 68;
    }
    else if (token == "hard" || token == "loud")
    {
        meta.minVelocity = 91;
        meta.maxVelocity = 127;
        meta.velocityValue = 110;
    }
}

bool SampleNameParser::applyRoundRobinToken (const juce::String& token, SampleMetadata& meta) const
{
    int rr = 0;

    if (token.startsWith ("rr"))
        rr = token.substring (2).getIntValue();
    else if (token.startsWith ("roundrobin"))
        rr = token.substring (10).getIntValue();
    else if (token.startsWith ("alt"))
        rr = token.substring (3).getIntValue();

    if (rr > 0)
    {
        meta.roundRobinIndex = rr;
        return true;
    }

    return false;
}

juce::String SampleNameParser::defaultArticulationForType (DrumPieceType type) const
{
    switch (type)
    {
        case DrumPieceType::kick:
        case DrumPieceType::snare:
        case DrumPieceType::rackTom:
        case DrumPieceType::floorTom:
        case DrumPieceType::crash:
        case DrumPieceType::china:
        case DrumPieceType::splash:
            return "Center";
        case DrumPieceType::hiHat:
            return "Closed";
        case DrumPieceType::ride:
            return "Bow";
        default:
            return "Hit";
    }
}

int SampleNameParser::defaultMidiForTypeAndArticulation (DrumPieceType type, int index,
                                                          const juce::String& articulation) const
{
    const auto art = normalizeToken (articulation);

    if (type == DrumPieceType::kick)
        return 36;

    if (type == DrumPieceType::snare)
    {
        if (art == "rimshot") return 40;
        if (art == "sidestick") return 37;
        return 38;
    }

    if (type == DrumPieceType::rackTom)
    {
        static constexpr int notes[] = { 48, 47, 45 };
        return notes[juce::jmin (index, 2)];
    }

    if (type == DrumPieceType::floorTom)
    {
        static constexpr int notes[] = { 43, 41 };
        return notes[juce::jmin (index, 1)];
    }

    if (type == DrumPieceType::hiHat)
    {
        if (art == "open") return 46;
        if (art == "pedal") return 44;
        return 42;
    }

    if (type == DrumPieceType::crash)
        return index == 0 ? 49 : 57;

    if (type == DrumPieceType::ride)
    {
        if (art == "bell") return 53;
        if (art == "edge") return 59;
        return 51;
    }

    if (type == DrumPieceType::china)
        return 52;

    if (type == DrumPieceType::splash)
        return 55;

    return 36 + index;
}

void SampleNameParser::finalizeMetadata (SampleMetadata& meta) const
{
    if (meta.minVelocity <= 0)
        meta.minVelocity = 1;

    if (meta.maxVelocity <= 0)
        meta.maxVelocity = 127;

    if (meta.articulation.isEmpty())
        meta.articulation = defaultArticulationForType (meta.instrumentType);

    if (meta.midiNote <= 0)
        meta.midiNote = defaultMidiForTypeAndArticulation (meta.instrumentType,
                                                           meta.instrumentIndex,
                                                           meta.articulation);

    meta.tags.add (drumPieceTypeToString (meta.instrumentType));
    meta.tags.add (meta.articulation.toLowerCase());
}
