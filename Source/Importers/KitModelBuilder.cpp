#include "KitModelBuilder.h"
#include "../Engine/Articulation.h"
#include "../Engine/DrumSample.h"
#include "../Engine/SampleLayer.h"
#include <algorithm>
#include <limits>
#include <map>
#include <vector>

namespace
{
    juce::String sanitizeArticulationName (const juce::String& name)
    {
        if (name.isEmpty())
            return "Hit";

        return name.substring (0, 1).toUpperCase() + name.substring (1);
    }

    bool isSinglePieceType (DrumPieceType type)
    {
        // Kits virtually always have one of each of these; collapsing them keeps
        // every variant/articulation on a single playable piece.
        return type == DrumPieceType::kick
            || type == DrumPieceType::snare
            || type == DrumPieceType::hiHat;
    }

    bool allDigits (const juce::String& s)
    {
        return s.isNotEmpty() && s.containsOnly ("0123456789");
    }

    bool startsWithDigits (const juce::String& s, const juce::String& prefix)
    {
        return s.startsWith (prefix) && s.length() > prefix.length()
            && s.substring (prefix.length()).containsOnly ("0123456789");
    }

    /** Tokens that describe articulation / velocity / round-robin / mic position
        rather than which physical drum this is. These are stripped when building
        a drum's grouping stem. */
    bool isDropToken (const juce::String& t)
    {
        if (allDigits (t))
            return false; // handled separately (merged onto the previous word)

        if (startsWithDigits (t, "rr") || startsWithDigits (t, "roundrobin")
            || startsWithDigits (t, "rrobin") || startsWithDigits (t, "alt")
            || startsWithDigits (t, "v"))
            return true;

        static const char* kDrop[] = {
            // velocity
            "soft", "softer", "medium", "med", "mid", "hard", "harder", "loud",
            "ghost", "vsoft", "vhard", "pp", "ppp", "mp", "mf", "ff", "fff",
            // mic / channel
            "stereo", "ster", "mono", "mon", "oh", "overhead", "room", "rooms",
            "close", "closemic", "far", "sum", "mix", "amb", "ambient", "direct",
            "dry", "wet", "left", "right", "lr",
            // articulation / technique
            "center", "centre", "head", "main", "hit", "hits", "edge", "bow",
            "tip", "bell", "shoulder", "shank", "rim", "rimshot", "rimclick",
            "side", "sidestick", "cross", "crossstick", "xstick", "stick",
            "open", "closed", "cl", "op", "pedal", "foot", "chick", "loose",
            "half", "mute", "muted", "choke", "choked", "reg", "regular",
            "normal", "norm", "nr", "hb", "sample", "wav"
        };

        for (auto* d : kDrop)
            if (t == d)
                return true;

        return false;
    }

    /** A stable identifier for "which drum" a sample belongs to, derived from the
        file name with articulation/velocity/RR/mic tokens removed. Adjacent voice
        numbers are merged onto the instrument word ("tom 1" -> "tom1"). */
    juce::String voiceStem (const juce::String& filePath)
    {
        const auto base = juce::File (filePath).getFileNameWithoutExtension().toLowerCase();
        const auto tokens = juce::StringArray::fromTokens (base, "_-. ()[]", "");

        juce::StringArray kept;

        for (auto token : tokens)
        {
            token = token.trim();

            if (token.isEmpty() || isDropToken (token))
                continue;

            if (allDigits (token))
            {
                // A size/voice number belongs to the preceding instrument word
                // (e.g. "crash" + "16"). A leading number with nothing before it
                // is a sequence prefix and is dropped.
                if (! kept.isEmpty())
                    kept.getReference (kept.size() - 1) += token;

                continue;
            }

            kept.add (token);
        }

        return kept.joinIntoString ("");
    }

    /** Tokens that vary between true round-robins / mic channels of the SAME strike
        (as opposed to a genuinely different drum body). Stripped to test whether two
        samples are the same voice. Note: unlike isDropToken, this KEEPS body/technique
        words (reg/nr/hb/hyb/center) so different drum bodies stay distinct. */
    bool isRoundRobinOrMicToken (const juce::String& t)
    {
        if (startsWithDigits (t, "rr") || startsWithDigits (t, "roundrobin")
            || startsWithDigits (t, "rrobin") || startsWithDigits (t, "alt")
            || startsWithDigits (t, "seq") || startsWithDigits (t, "v"))
            return true;

        static const char* kDrop[] = {
            "stereo", "ster", "mono", "mon", "oh", "overhead", "room", "rooms",
            "close", "closemic", "far", "sum", "mix", "amb", "ambient", "direct",
            "dry", "wet", "left", "right", "lr", "l", "r", "mic", "mics", "ch",
            "sample", "wav"
        };

        for (auto* d : kDrop)
            if (t == d)
                return true;

        return false;
    }

    /** Identifies "which physical drum/strike" a sample is, ignoring only round-robin
        index, velocity prefix and mic channel. Two samples with the same stem are
        genuine variants of one voice; different stems are different drums. */
    juce::String voiceIdentity (const juce::String& filePath)
    {
        const auto base = juce::File (filePath).getFileNameWithoutExtension().toLowerCase();
        const auto tokens = juce::StringArray::fromTokens (base, "_-. ()[]", "");

        juce::StringArray kept;

        for (int i = 0; i < tokens.size(); ++i)
        {
            const auto t = tokens[i].trim();

            if (t.isEmpty())
                continue;

            if (i == 0 && allDigits (t))            // leading velocity / take index
                continue;

            if (isRoundRobinOrMicToken (t))
                continue;

            if (allDigits (t))                      // body/size number -> merge onto word
            {
                if (! kept.isEmpty())
                    kept.getReference (kept.size() - 1) += t;

                continue;
            }

            kept.add (t);
        }

        return kept.joinIntoString ("");
    }

    juce::String groupKeyFor (const SampleMetadata& meta)
    {
        const auto typeStr = drumPieceTypeToString (meta.instrumentType);

        // Always one kick/snare/hi-hat, regardless of how it was grouped upstream.
        if (isSinglePieceType (meta.instrumentType))
            return typeStr;

        // Prefer an LLM-assigned grouping key when present (robust to vendor naming).
        if (meta.pieceGroupKey.isNotEmpty())
            return typeStr + "|" + meta.pieceGroupKey;

        return typeStr + "|" + voiceStem (meta.filePath);
    }

    void applyPieceDefaults (DrumPiece& piece, DrumPieceType type)
    {
        piece.type = type;
        piece.name = defaultPieceDisplayName (type);
        piece.width = defaultPieceSize (type);
        piece.height = defaultPieceSize (type);
        normalizePieceVisuals (piece);

        if (type == DrumPieceType::hiHat)
            piece.chokeGroupId = "hihat";
    }

    void addSampleToModel (KitModel& model,
                           juce::HashMap<juce::String, juce::String>& pieceIdsByGroup,
                           const SampleMetadata& meta,
                           std::vector<ImportWarning>& warnings)
    {
        const auto groupKey = groupKeyFor (meta);

        DrumPiece* piece = nullptr;
        const auto existingId = pieceIdsByGroup[groupKey];

        if (existingId.isNotEmpty())
        {
            piece = model.findPieceById (existingId);
        }
        else
        {
            DrumPiece newPiece;
            newPiece.id = DrumPiece::makeId();
            applyPieceDefaults (newPiece, meta.instrumentType);
            newPiece.primaryMidiNote = meta.midiNote;

            auto& added = model.addPiece (std::move (newPiece));
            pieceIdsByGroup.set (groupKey, added.id);
            piece = model.findPieceById (added.id);
        }

        if (piece == nullptr)
            return;

        const auto artName = sanitizeArticulationName (meta.articulation);
        Articulation* art = nullptr;

        for (auto& existing : piece->articulations)
        {
            if (existing.name.equalsIgnoreCase (artName))
            {
                art = &existing;
                break;
            }
        }

        if (art == nullptr)
        {
            Articulation newArt;
            newArt.id = Articulation::makeId();
            newArt.name = artName;
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

        for (const auto& existing : layer->roundRobins.samples)
            if (existing.filePath == meta.filePath)
                return;

        DrumSample sample;
        sample.id = DrumSample::makeId();
        sample.filePath = meta.filePath;
        sample.rootMidiNote = meta.midiNote;

        if (! juce::File (meta.filePath).existsAsFile())
            warnings.push_back (ImportWarning::make (ImportWarningType::sampleFileMissing,
                                                     "Sample file missing: " + meta.filePath,
                                                     meta.filePath));

        if (meta.roundRobinIndex > 0)
        {
            auto& rrSamples = layer->roundRobins.samples;
            const int insertAt = juce::jmax (0, meta.roundRobinIndex - 1);

            if (insertAt >= (int) rrSamples.size())
                rrSamples.push_back (std::move (sample));
            else
                rrSamples.insert (rrSamples.begin() + insertAt, std::move (sample));
        }
        else
        {
            layer->roundRobins.addSample (std::move (sample));
        }

        piece->syncMidiNotesFromArticulations();
    }

    int primaryNote (const DrumPiece& piece)
    {
        if (piece.primaryMidiNote > 0)
            return piece.primaryMidiNote;

        if (! piece.articulations.empty())
            return piece.articulations.front().midiNote;

        return 0;
    }

    void setTomType (DrumPiece& p, DrumPieceType target)
    {
        if (p.type == target)
            return;

        p.type = target;
        p.width = defaultPieceSize (target);
        p.height = defaultPieceSize (target);
        normalizePieceVisuals (p);
    }

    /** Decides rack vs floor by PITCH (lowest = floor), which is reliable, instead
        of trusting upstream name guesses (the LLM/resolver often mislabels which
        tom is the floor). We keep the upstream *count* of floor toms when it gave
        one, otherwise fall back to a standard-kit heuristic. */
    void splitTomsByPitch (KitModel& model)
    {
        std::vector<std::pair<int, juce::String>> toms;
        int upstreamFloorCount = 0;

        for (const auto& p : model.getPieces())
        {
            if (p.type == DrumPieceType::rackTom || p.type == DrumPieceType::floorTom)
            {
                toms.push_back ({ primaryNote (p), p.id });

                if (p.type == DrumPieceType::floorTom)
                    ++upstreamFloorCount;
            }
        }

        if (toms.size() < 2)
            return;

        std::sort (toms.begin(), toms.end(),
                   [] (const auto& a, const auto& b) { return a.first < b.first; });

        const int n = (int) toms.size();
        const int heuristic = n >= 5 ? 2 : 1;
        const int floorCount = juce::jlimit (1, n, upstreamFloorCount > 0 ? upstreamFloorCount : heuristic);

        for (int i = 0; i < n; ++i)
            if (auto* p = model.findPieceById (toms[(size_t) i].second))
                setTomType (*p, i < floorCount ? DrumPieceType::floorTom : DrumPieceType::rackTom);
    }

    /** Over-eager grouping (especially the LLM collapsing kick/snare/hi-hat to one
        piece) can pile several DIFFERENT drum bodies into one articulation's
        round-robin pool, so clicking it alternates between dissimilar samples. Keep
        only the dominant voice (most samples) per articulation; drop the rest. */
    void pruneMixedVoicesImpl (KitModel& model)
    {
        std::vector<juce::String> ids;

        for (const auto& p : model.getPieces())
            ids.push_back (p.id);

        for (const auto& id : ids)
        {
            auto* piece = model.findPieceById (id);

            if (piece == nullptr)
                continue;

            for (auto& art : piece->articulations)
            {
                std::map<juce::String, int> counts;

                for (auto& layer : art.layers)
                    for (auto& s : layer.roundRobins.samples)
                        ++counts[voiceIdentity (s.filePath)];

                if (counts.size() <= 1)
                    continue;

                juce::String dominant;
                int best = -1;

                for (const auto& kv : counts)
                    if (kv.second > best)
                    {
                        best = kv.second;
                        dominant = kv.first;
                    }

                for (auto& layer : art.layers)
                {
                    auto& samps = layer.roundRobins.samples;
                    samps.erase (std::remove_if (samps.begin(), samps.end(),
                                                 [&] (const DrumSample& s)
                                                 { return voiceIdentity (s.filePath) != dominant; }),
                                 samps.end());
                }

                art.layers.erase (std::remove_if (art.layers.begin(), art.layers.end(),
                                                  [] (const SampleLayer& l)
                                                  { return l.roundRobins.samples.empty(); }),
                                  art.layers.end());
            }
        }
    }

    /** Lower = more likely to be the "default" hit you hear when clicking a piece. */
    int articulationRank (DrumPieceType type, const juce::String& name)
    {
        const auto n = name.toLowerCase();

        if (type == DrumPieceType::hiHat)
        {
            if (n.contains ("closed") || n == "cl" || n.contains ("tight")) return 0;
            if (n.contains ("tip") || n.contains ("bow") || n.contains ("edge")) return 1;
            if (n.contains ("open"))  return 2;
            if (n.contains ("pedal") || n.contains ("foot") || n.contains ("chick")) return 3;
            return 4;
        }

        // Drums and cymbals: prefer the plain main hit; push secondary
        // techniques (rim/sidestick/bell) to the back.
        if (n.contains ("center") || n.contains ("centre")) return 0;
        if (n == "hit" || n.contains ("bow") || n.contains ("tip")) return 1;
        if (n.contains ("open"))  return 2;
        if (n.contains ("edge"))  return 3;
        if (n.contains ("bell"))  return 5;
        if (n.contains ("rim"))   return 6;
        if (n.contains ("side") || n.contains ("cross") || n.contains ("stick")) return 7;
        return 4;
    }

    /** Makes each piece's primary articulation the natural default (snare -> Center,
        hi-hat -> Closed, etc.) so clicking the piece plays the expected sample. */
    void assignPrimaryArticulations (KitModel& model)
    {
        std::vector<juce::String> ids;

        for (const auto& p : model.getPieces())
            ids.push_back (p.id);

        for (const auto& id : ids)
        {
            auto* piece = model.findPieceById (id);

            if (piece == nullptr || piece->articulations.empty())
                continue;

            size_t best = 0;
            int bestRank = std::numeric_limits<int>::max();

            for (size_t i = 0; i < piece->articulations.size(); ++i)
            {
                const int r = articulationRank (piece->type, piece->articulations[i].name);

                if (r < bestRank)
                {
                    bestRank = r;
                    best = i;
                }
            }

            if (best != 0)
                std::swap (piece->articulations[0], piece->articulations[best]);

            piece->primaryMidiNote = piece->articulations[0].midiNote;
            piece->syncMidiNotesFromArticulations();
        }
    }

    /** Guarantees every articulation in the kit triggers a unique MIDI note.
        The first articulation to claim a note keeps it; later duplicates move to
        the nearest free note, so two same-type pieces (e.g. a second ride) never
        share a trigger. Pieces are processed in model order. */
    void ensureUniqueArticulationNotes (KitModel& model)
    {
        bool used[128] = { false };

        auto claimNearestFree = [&used] (int desired) -> int
        {
            const int start = juce::jlimit (0, 127, desired);

            for (int delta = 0; delta < 128; ++delta)
            {
                if (const int up = start + delta; up <= 127 && ! used[up])
                    return up;

                if (const int down = start - delta; down >= 0 && ! used[down])
                    return down;
            }

            return start;
        };

        for (auto& piece : model.getPiecesMutable())
        {
            for (auto& art : piece.articulations)
            {
                int note = juce::jlimit (0, 127, art.midiNote);

                if (used[note])
                    note = claimNearestFree (note + 1);

                art.midiNote = note;
                used[note] = true;
            }

            piece.syncMidiNotesFromArticulations();

            if (! piece.articulations.empty())
                piece.primaryMidiNote = piece.articulations[0].midiNote;
        }
    }

    /** Assigns clean display names. Multi-instance types are numbered by pitch. */
    void renamePieces (KitModel& model)
    {
        struct Spec { DrumPieceType type; bool alwaysNumber; };

        static const Spec specs[] = {
            { DrumPieceType::kick,     false },
            { DrumPieceType::snare,    false },
            { DrumPieceType::hiHat,    false },
            { DrumPieceType::rackTom,  true  },
            { DrumPieceType::floorTom, true  },
            { DrumPieceType::crash,    true  },
            { DrumPieceType::ride,     true  },
            { DrumPieceType::china,    false },
            { DrumPieceType::splash,   false },
            { DrumPieceType::accessory, true }
        };

        for (const auto& spec : specs)
        {
            std::vector<std::pair<int, juce::String>> items;

            for (const auto& p : model.getPieces())
                if (p.type == spec.type)
                    items.push_back ({ primaryNote (p), p.id });

            std::sort (items.begin(), items.end(),
                       [] (const auto& a, const auto& b) { return a.first < b.first; });

            const bool number = spec.alwaysNumber || items.size() > 1;

            for (size_t i = 0; i < items.size(); ++i)
            {
                if (auto* p = model.findPieceById (items[i].second))
                {
                    p->name = number
                                  ? defaultPieceDisplayName (spec.type) + " " + juce::String ((int) i + 1)
                                  : defaultPieceDisplayName (spec.type);
                }
            }
        }
    }
}

KitModel KitModelBuilder::buildFromMetadata (const juce::String& kitName,
                                             const std::vector<SampleMetadata>& samples,
                                             std::vector<ImportWarning>& warnings)
{
    KitModel model = KitModel::createEmpty();
    model.kitName = kitName;

    juce::HashMap<juce::String, juce::String> pieceIdsByGroup;

    for (const auto& meta : samples)
    {
        if (meta.confidence <= 0.0f && meta.instrumentType == DrumPieceType::accessory)
            warnings.push_back (ImportWarning::make (ImportWarningType::couldNotInferInstrument,
                                                     "Could not infer instrument for: "
                                                         + juce::File (meta.filePath).getFileName(),
                                                     meta.filePath));

        addSampleToModel (model, pieceIdsByGroup, meta, warnings);
    }

    splitTomsByPitch (model);
    pruneMixedVoicesImpl (model);
    assignPrimaryArticulations (model);
    ensureUniqueArticulationNotes (model);
    renamePieces (model);

    return model;
}

void KitModelBuilder::pruneMixedVoices (KitModel& model)
{
    pruneMixedVoicesImpl (model);
}

void KitModelBuilder::ensureUniqueMidiNotes (KitModel& model)
{
    ensureUniqueArticulationNotes (model);
}
