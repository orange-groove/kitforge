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

    int matchScore (const SampleLibraryMapper::SourceEntry& source,
                    const SampleLibraryMapper::TargetSlot& target,
                    const KitModel& targetKit,
                    const KitModel& libraryCatalog)
    {
        const auto* targetPiece = targetKit.findPieceById (target.pieceId);
        const auto* sourcePiece = libraryCatalog.findPieceById (source.sourcePieceId);

        if (targetPiece == nullptr || sourcePiece == nullptr)
            return 0;

        int score = 0;

        if (targetPiece->type == sourcePiece->type)
            score += 4;

        if (targetPiece->type == source.type)
            score += 2;

        if (normalizeName (targetPiece->name) == normalizeName (source.pieceName))
            score += 5;

        if (normalizeName (target.label).contains (normalizeName (source.articulationName)))
            score += 4;

        if (normalizeName (source.articulationName) == "center"
            && normalizeName (target.label).contains ("center"))
            score += 3;

        if (normalizeName (source.articulationName) == "closed"
            && normalizeName (target.label).contains ("closed"))
            score += 4;

        if (normalizeName (source.articulationName) == "open"
            && normalizeName (target.label).contains ("open"))
            score += 4;

        if (normalizeName (source.articulationName) == "bow"
            && normalizeName (target.label).contains ("bow"))
            score += 4;

        if (normalizeName (source.articulationName) == "hit"
            && (normalizeName (target.label).contains ("hit")
                || normalizeName (target.label).contains ("center")))
            score += 2;

        const auto* targetArt = targetPiece->findArticulationById (target.articulationId);

        if (targetArt != nullptr && targetArt->midiNote == source.midiNote)
            score += 5;

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
    juce::StringArray usedTargets;

    for (const auto& source : sources)
    {
        Mapping mapping;
        mapping.sourceKey = source.key;
        mapping.enabled = source.sampleCount > 0;

        int bestScore = 0;
        juce::String bestTargetKey;

        for (const auto& target : targets)
        {
            if (usedTargets.contains (target.key))
                continue;

            const int score = matchScore (source, target, targetKit, libraryCatalog);

            if (score > bestScore)
            {
                bestScore = score;
                bestTargetKey = target.key;
            }
        }

        if (bestScore >= 6)
        {
            mapping.targetKey = bestTargetKey;
            usedTargets.add (bestTargetKey);
        }

        mappings.add (std::move (mapping));
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
        targetKit.notifyChanged();

    return applied;
}
