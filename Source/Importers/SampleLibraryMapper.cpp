#include "SampleLibraryMapper.h"
#include "../Engine/Articulation.h"

namespace
{
    juce::String normalizeName (const juce::String& name)
    {
        return name.trim().toLowerCase().removeCharacters ("-_");
    }

    int countSamplesInArticulation (const Articulation& art)
    {
        int count = 0;

        for (const auto& layer : art.layers)
            count += (int) layer.roundRobins.samples.size();

        return count;
    }

    void copySampleLayers (Articulation& target, const Articulation& source)
    {
        target.layers.clear();

        for (const auto& layer : source.layers)
        {
            SampleLayer copy = layer;
            copy.id = SampleLayer::makeId();

            for (auto& sample : copy.roundRobins.samples)
                sample.id = DrumSample::makeId();

            target.layers.push_back (std::move (copy));
        }
    }

    struct SampleProfile
    {
        int bell = 0;
        int rideBow = 0;
        int china = 0;
        int crash = 0;
        int total = 0;
    };

    SampleProfile profileArticulation (const Articulation& art)
    {
        SampleProfile profile;

        for (const auto& layer : art.layers)
        {
            for (const auto& sample : layer.roundRobins.samples)
            {
                const auto lower = sample.filePath.toLowerCase();
                ++profile.total;

                if (lower.contains ("bell"))
                    ++profile.bell;
                else if (lower.contains ("china"))
                    ++profile.china;
                else if (lower.contains ("crash"))
                    ++profile.crash;
                else if (lower.contains ("ride"))
                    ++profile.rideBow;
            }
        }

        return profile;
    }

    bool isCymbalType (DrumPieceType type)
    {
        switch (type)
        {
            case DrumPieceType::hiHat:
            case DrumPieceType::crash:
            case DrumPieceType::ride:
            case DrumPieceType::china:
            case DrumPieceType::splash:
                return true;
            default:
                return false;
        }
    }

    bool profileMatchesTarget (const SampleProfile& profile,
                               DrumPieceType targetType,
                               const juce::String& targetLabel)
    {
        if (profile.total <= 0)
            return false;

        const auto label = normalizeName (targetLabel);

        if (targetType == DrumPieceType::ride)
        {
            if (label.contains ("bell"))
                return profile.bell > 0 && profile.china == 0 && profile.rideBow == 0;

            if (label.contains ("edge") || label.contains ("bow"))
                return profile.rideBow > 0 && profile.bell == 0 && profile.china == 0;
        }

        if (targetType == DrumPieceType::crash)
            return profile.crash > 0 || (profile.bell == 0 && profile.rideBow == 0 && profile.china == 0);

        if (targetType == DrumPieceType::china)
            return profile.china > 0 && profile.rideBow == 0 && profile.bell == 0;

        return true;
    }

    int matchScore (const SampleLibraryMapper::SourceEntry& source,
                    const SampleLibraryMapper::TargetSlot& target,
                    const KitModel& targetKit,
                    const KitModel& libraryCatalog)
    {
        const auto* targetPiece = targetKit.findPieceById (target.pieceId);
        const auto* sourcePiece = libraryCatalog.findPieceById (source.sourcePieceId);
        const auto* sourceArt = sourcePiece != nullptr
                                    ? sourcePiece->findArticulationById (source.sourceArticulationId)
                                    : nullptr;

        if (targetPiece == nullptr || sourcePiece == nullptr || sourceArt == nullptr)
            return 0;

        const auto profile = profileArticulation (*sourceArt);
        const auto targetLabel = normalizeName (target.label);

        if (! profileMatchesTarget (profile, targetPiece->type, target.label))
            return 0;

        if (isCymbalType (targetPiece->type) && targetPiece->type != sourcePiece->type)
        {
            const bool crossTypeRideEdge = targetPiece->type == DrumPieceType::ride
                                           && (targetLabel.contains ("edge") || targetLabel.contains ("bow"))
                                           && profile.rideBow > 0;

            if (! crossTypeRideEdge)
                return 0;
        }

        int score = 0;

        if (targetPiece->type == sourcePiece->type)
            score += 6;

        if (targetPiece->type == source.type)
            score += 2;

        if (normalizeName (targetPiece->name) == normalizeName (source.pieceName))
            score += 5;

        if (targetLabel.contains (normalizeName (source.articulationName)))
            score += 4;

        if (normalizeName (source.articulationName) == "center"
            && targetLabel.contains ("center"))
            score += 3;

        if (normalizeName (source.articulationName) == "closed"
            && targetLabel.contains ("closed"))
            score += 4;

        if (normalizeName (source.articulationName) == "open"
            && targetLabel.contains ("open"))
            score += 4;

        if (normalizeName (source.articulationName) == "bow"
            && (targetLabel.contains ("bow") || targetLabel.contains ("edge")))
            score += 6;

        if (normalizeName (source.articulationName) == "bell"
            && targetLabel.contains ("bell"))
            score += 8;

        if (normalizeName (source.articulationName) == "edge"
            && targetLabel.contains ("edge"))
            score += 6;

        if (targetPiece->type == DrumPieceType::ride && targetLabel.contains ("bell") && profile.bell > 0)
            score += 10;

        if (targetPiece->type == DrumPieceType::ride
            && (targetLabel.contains ("edge") || targetLabel.contains ("bow"))
            && profile.rideBow > 0)
            score += 10;

        if (targetPiece->type == DrumPieceType::crash && profile.crash > 0)
            score += 8;

        if (targetPiece->type == DrumPieceType::china && profile.china > 0)
            score += 8;

        const auto* targetArt = targetPiece->findArticulationById (target.articulationId);

        if (targetArt != nullptr && targetArt->midiNote == source.midiNote)
            score += 3;

        return score;
    }
}

juce::String SampleLibraryMapper::makeKey (const juce::String& pieceId, const juce::String& articulationId)
{
    return pieceId + "|" + articulationId;
}

juce::Array<SampleLibraryMapper::SourceEntry> SampleLibraryMapper::listLibrarySources (const KitModel& libraryCatalog)
{
    juce::Array<SourceEntry> entries;

    for (const auto& piece : libraryCatalog.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            SourceEntry entry;
            entry.key = makeKey (piece.id, art.id);
            entry.pieceName = piece.name;
            entry.articulationName = art.name;
            entry.type = piece.type;
            entry.midiNote = art.midiNote;
            entry.sampleCount = countSamplesInArticulation (art);
            entry.sourcePieceId = piece.id;
            entry.sourceArticulationId = art.id;
            entries.add (std::move (entry));
        }
    }

    return entries;
}

juce::Array<SampleLibraryMapper::TargetSlot> SampleLibraryMapper::listTargetSlots (const KitModel& targetKit)
{
    juce::Array<TargetSlot> slots;

    for (const auto& piece : targetKit.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            TargetSlot slot;
            slot.pieceId = piece.id;
            slot.articulationId = art.id;
            slot.key = makeKey (piece.id, art.id);
            slot.label = piece.name + " / " + art.name + " (MIDI " + juce::String (art.midiNote) + ")";
            slots.add (std::move (slot));
        }
    }

    return slots;
}

juce::Array<SampleLibraryMapper::Mapping> SampleLibraryMapper::suggestMappings (const KitModel& targetKit,
                                                                                 const KitModel& libraryCatalog)
{
    const auto sources = listLibrarySources (libraryCatalog);
    const auto targets = listTargetSlots (targetKit);
    juce::Array<Mapping> mappings;
    juce::StringArray usedSources;

    for (const auto& source : sources)
    {
        Mapping mapping;
        mapping.sourceKey = source.key;
        mapping.enabled = source.sampleCount > 0;
        mappings.add (std::move (mapping));
    }

    for (const auto& target : targets)
    {
        int bestScore = 0;
        juce::String bestSourceKey;

        for (const auto& source : sources)
        {
            if (usedSources.contains (source.key) || source.sampleCount <= 0)
                continue;

            const int score = matchScore (source, target, targetKit, libraryCatalog);

            if (score > bestScore)
            {
                bestScore = score;
                bestSourceKey = source.key;
            }
        }

        if (bestScore < 8 || bestSourceKey.isEmpty())
            continue;

        usedSources.add (bestSourceKey);

        for (auto& mapping : mappings)
        {
            if (mapping.sourceKey == bestSourceKey)
            {
                mapping.targetKey = target.key;
                break;
            }
        }
    }

    return mappings;
}

int SampleLibraryMapper::applyMappings (KitModel& targetKit,
                                         const KitModel& libraryCatalog,
                                         const juce::Array<Mapping>& mappings)
{
    int applied = 0;

    for (const auto& mapping : mappings)
    {
        if (! mapping.enabled || mapping.targetKey.isEmpty())
            continue;

        const int sourceSep = mapping.sourceKey.indexOfChar ('|');
        const int targetSep = mapping.targetKey.indexOfChar ('|');

        if (sourceSep <= 0 || targetSep <= 0)
            continue;

        const auto sourcePieceId = mapping.sourceKey.substring (0, sourceSep);
        const auto sourceArtId = mapping.sourceKey.substring (sourceSep + 1);
        const auto targetPieceId = mapping.targetKey.substring (0, targetSep);
        const auto targetArtId = mapping.targetKey.substring (targetSep + 1);

        const auto* sourcePiece = libraryCatalog.findPieceById (sourcePieceId);

        if (sourcePiece == nullptr)
            continue;

        const auto* sourceArt = sourcePiece->findArticulationById (sourceArtId);

        if (sourceArt == nullptr || sourceArt->layers.empty())
            continue;

        auto* targetPiece = targetKit.findPieceById (targetPieceId);

        if (targetPiece == nullptr)
            continue;

        auto* targetArt = targetPiece->findArticulationById (targetArtId);

        if (targetArt == nullptr)
            continue;

        copySampleLayers (*targetArt, *sourceArt);
        targetPiece->syncMidiNotesFromArticulations();
        ++applied;
    }

    if (applied > 0)
    {
        targetKit.normalizeStandardArticulations();
        targetKit.notifyChanged();
    }

    return applied;
}
