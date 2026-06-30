#include "LlmSampleClassifier.h"
#include "../AI/OpenAIChat.h"
#include "../Core/KitForgeSettings.h"
#include "../Core/KitForgePaths.h"
#include <set>

namespace
{
    constexpr int kMaxClasses = 200;

    bool allDigits (const juce::String& s)
    {
        return s.isNotEmpty() && s.containsOnly ("0123456789");
    }

    bool startsWithDigits (const juce::String& s, const juce::String& prefix)
    {
        return s.startsWith (prefix) && s.length() > prefix.length()
            && s.substring (prefix.length()).containsOnly ("0123456789");
    }

    /** Collapses a filename to its instrument+articulation pattern by removing the
        leading velocity index and round-robin suffix (the parts that vary between
        otherwise-identical samples). Keeps descriptive case ("Cnt", "Bell"). */
    juce::String classNameFor (const juce::String& filePath)
    {
        const auto base = juce::File (filePath).getFileNameWithoutExtension();
        const auto tokens = juce::StringArray::fromTokens (base, "_-. ()[]", "");

        juce::StringArray kept;

        for (int i = 0; i < tokens.size(); ++i)
        {
            const auto tok = tokens[i].trim();

            if (tok.isEmpty())
                continue;

            const auto low = tok.toLowerCase();

            if (i == 0 && allDigits (tok))     // leading velocity / take index
                continue;

            if (allDigits (tok))               // stray numeric token
                continue;

            if (startsWithDigits (low, "rr") || startsWithDigits (low, "roundrobin")
                || startsWithDigits (low, "alt") || startsWithDigits (low, "v"))
                continue;

            kept.add (tok);
        }

        return kept.isEmpty() ? base : kept.joinIntoString ("_");
    }

    struct SampleClass
    {
        juce::String name;
        juce::String folder;
        std::set<int> midiNotes;
        std::set<int> layerNumbers;
        juce::StringArray examples;
        int count = 0;
        std::vector<size_t> sampleIndices;
    };

    std::vector<SampleClass> buildClasses (const std::vector<SampleMetadata>& samples)
    {
        std::vector<SampleClass> classes;
        juce::HashMap<juce::String, int> indexByName;

        for (size_t i = 0; i < samples.size(); ++i)
        {
            const auto& meta = samples[i];
            const auto name = classNameFor (meta.filePath);

            int idx;

            if (indexByName.contains (name))
            {
                idx = indexByName[name];
            }
            else
            {
                idx = (int) classes.size();
                indexByName.set (name, idx);
                SampleClass sc;
                sc.name = name;
                sc.folder = juce::File (meta.filePath).getParentDirectory().getFileName();
                classes.push_back (std::move (sc));
            }

            auto& sc = classes[(size_t) idx];
            ++sc.count;
            sc.sampleIndices.push_back (i);

            if (sc.examples.size() < 3)
                sc.examples.add (juce::File (meta.filePath).getFileName());

            if (meta.layerIndex > 0)
                sc.layerNumbers.insert (meta.layerIndex);

            if (meta.midiNote > 0)
                sc.midiNotes.insert (meta.midiNote);
        }

        return classes;
    }

    juce::String buildUserMessage (const juce::String& kitName, const std::vector<SampleClass>& classes)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("kit", kitName);

        juce::Array<juce::var> arr;

        for (size_t i = 0; i < classes.size(); ++i)
        {
            const auto& sc = classes[i];
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("ref", (int) i);
            obj->setProperty ("name", sc.name);
            obj->setProperty ("folder", sc.folder);

            juce::Array<juce::var> midi;
            for (int note : sc.midiNotes)
                midi.add (note);

            juce::Array<juce::var> layers;
            for (int layer : sc.layerNumbers)
                layers.add (layer);

            juce::Array<juce::var> examples;
            for (const auto& ex : sc.examples)
                examples.add (ex);

            obj->setProperty ("midi", midi);
            obj->setProperty ("layerNumbers", layers);
            obj->setProperty ("examples", examples);
            obj->setProperty ("count", sc.count);
            arr.add (juce::var (obj));
        }

        root->setProperty ("classes", arr);
        return juce::JSON::toString (juce::var (root));
    }

    /** Tolerant of casing/spelling variants the model may emit (e.g. "hihat",
        "hi-hat", "tom", "floor"); strict drumPieceTypeFromString would drop these
        to accessory. */
    DrumPieceType parseType (const juce::String& raw)
    {
        const auto s = raw.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz");

        if (s == "kick" || s == "kik" || s == "bassdrum" || s == "bd" || s == "bassdrm")
            return DrumPieceType::kick;
        if (s == "snare" || s == "snr" || s == "sd")
            return DrumPieceType::snare;
        if (s == "hihat" || s == "hat" || s == "hats" || s == "hh" || s == "hihats")
            return DrumPieceType::hiHat;
        if (s == "floortom" || s == "floor" || s == "ft" || s == "fttom")
            return DrumPieceType::floorTom;
        if (s == "racktom" || s == "rack" || s == "tom" || s == "toms" || s == "tomtom"
            || s == "hightom" || s == "midtom" || s == "lowtom")
            return DrumPieceType::rackTom;
        if (s == "crash" || s == "crashcymbal")
            return DrumPieceType::crash;
        if (s == "ride" || s == "ridecymbal")
            return DrumPieceType::ride;
        if (s == "china")
            return DrumPieceType::china;
        if (s == "splash")
            return DrumPieceType::splash;

        return DrumPieceType::accessory;
    }

    struct Assignment
    {
        DrumPieceType type = DrumPieceType::accessory;
        juce::String piece;
        juce::String articulation;
        juce::String layerScheme;
        bool set = false;
    };

    /** Parses {"map":[{ref,type,piece,articulation}]} and fills assignments by ref.
        Returns false unless every class ref is covered exactly. */
    bool parseMap (const juce::String& json, int expectedCount, std::vector<Assignment>& out)
    {
        juce::var parsed;

        if (juce::JSON::parse (json, parsed).failed())
            return false;

        auto* mapArray = parsed.getProperty ("map", juce::var()).getArray();

        if (mapArray == nullptr)
            return false;

        out.assign ((size_t) expectedCount, Assignment());

        for (const auto& entry : *mapArray)
        {
            const int ref = (int) entry.getProperty ("ref", -1);

            if (ref < 0 || ref >= expectedCount)
                continue;

            Assignment a;
            a.type = parseType (entry.getProperty ("type", "accessory").toString());
            a.piece = entry.getProperty ("piece", "").toString().trim();
            a.articulation = entry.getProperty ("articulation", "").toString().trim();
            a.layerScheme = entry.getProperty ("layerScheme", "auto").toString().trim().toLowerCase();
            a.set = true;
            out[(size_t) ref] = a;
        }

        for (const auto& a : out)
            if (! a.set)
                return false;

        return true;
    }

    juce::File cacheFileFor (const juce::String& kitName, const std::vector<SampleClass>& classes)
    {
        juce::StringArray names;

        for (const auto& sc : classes)
            names.add (sc.name + "|" + sc.folder);

        names.sort (false);

        const auto keyText = "classify-v2\n" + kitName + "\n" + names.joinIntoString ("\n");
        const auto hash = juce::String ((juce::int64) keyText.hashCode64());

        return KitForgePaths::getKitForgeRoot()
                   .getChildFile ("Cache")
                   .getChildFile ("classify")
                   .getChildFile (hash + ".json");
    }
}

juce::String LlmSampleClassifier::systemPrompt()
{
    return R"(You are a drum-library librarian. You are given the distinct sample-name patterns from ONE drum library. Group them into the physical instruments of a drum kit and label each pattern with its articulation.

Reply with ONLY valid JSON, no prose:
{ "map": [ { "ref": number, "type": string, "piece": string, "articulation": string, "layerScheme": string } ] }

FIELDS
- "type": kick, snare, rackTom, floorTom, hiHat, crash, ride, china, splash, or accessory.
- "piece": STABLE grouping key. Same piece = one drum with multiple articulations. Different piece = separate drums.
- "articulation": short label (Center, Bow, Bell, Edge, Rimshot, Sidestick, Closed, Open, Pedal, etc.).
- "layerScheme": how trailing layer numbers behave for this pattern:
    "auto"       — default; KitForge applies Kontakt-style rules from layerNumbers
    "roundRobin" — all layer numbers are round-robin variants of one velocity range
    "velocity"   — each layer number is a separate velocity layer
  Use "auto" unless the examples clearly show only RR or only velocity layering.

INPUT HINTS
Each class includes "examples" (real filenames) and "layerNumbers" (trailing indices found).
Native Instruments / Kontakt often uses: "{Instrument} - {Part} - {Layer}.wav"
  - The trailing number is NOT a separate drum — it belongs to the same ref/piece+articulation.
  - layerNumbers like [1,2,3,4,5,6,7,8] on one class are layers of ONE articulation, not eight drums.

GROUPING GUIDANCE
- kick, snare, hiHat: usually ONE piece each (piece "kick", "snare", "hihat") unless clearly distinct bodies (e.g. "bop kick" = piece "bop-kick", type kick).
- Snare techniques (rimshot, sidestick, cross stick, stickshot) = articulations on piece "snare", type snare.
- Hi-hat: closed/tight = Closed, open = Open, foot/pedal = Pedal — one piece "hihat".
- Cymbal sizes distinguish different cymbals: separate pieces (e.g. "crash-16", "ride-20", "flat-ride").
- Cymbal zones (bow/center, bell, edge, crash) = articulations sharing the cymbal's piece.
- Toms: separate piece per drum; rackTom vs floorTom by size/pitch (lower = floorTom).
- IGNORE mic tokens (L, R, stereo, OH, room) — they do not create new pieces.

- Output exactly one entry for EVERY ref in the input. Do not invent refs.

WORKED EXAMPLE (underscore naming)
Input: name "hihat_closed", examples ["hihat - closed - 1.wav"], layerNumbers [1..8], count 8
Output: { ref:N, type:"hiHat", piece:"hihat", articulation:"Closed", layerScheme:"auto" }

Input: name "bop_kick_snares_off", examples ["bop kick - snares off - 2.wav"]
Output: { ref:N, type:"kick", piece:"bop-kick", articulation:"Bop Snares Off", layerScheme:"auto" })";
}

bool LlmSampleClassifier::classify (const juce::String& kitName, std::vector<SampleMetadata>& samples)
{
    if (samples.empty())
        return false;

    const auto classes = buildClasses (samples);

    if (classes.empty() || (int) classes.size() > kMaxClasses)
        return false;

    const auto cacheFile = cacheFileFor (kitName, classes);
    std::vector<Assignment> assignments;
    bool haveAssignments = false;

    if (cacheFile.existsAsFile())
        haveAssignments = parseMap (cacheFile.loadFileAsString(), (int) classes.size(), assignments);

    if (! haveAssignments)
    {
        const auto apiKey = KitForgeSettings::get().getOpenAiApiKey();
        const auto model = KitForgeSettings::get().getOpenAiModel();

        if (apiKey.trim().isEmpty())
            return false;

        const auto userMessage = buildUserMessage (kitName, classes);
        const auto response = OpenAIChat::complete (systemPrompt(), userMessage, apiKey, model, 0.0);

        if (! response.success)
            return false;

        const auto json = OpenAIChat::extractJsonContent (response.content);

        if (! parseMap (json, (int) classes.size(), assignments))
            return false;

        cacheFile.getParentDirectory().createDirectory();
        cacheFile.replaceWithText (json);
        haveAssignments = true;
    }

    for (size_t i = 0; i < classes.size(); ++i)
    {
        const auto& a = assignments[i];

        for (auto sampleIndex : classes[i].sampleIndices)
        {
            auto& meta = samples[sampleIndex];
            meta.instrumentType = a.type;
            meta.pieceGroupKey = a.piece;
            meta.confidence = 1.0f;

            if (a.articulation.isNotEmpty())
                meta.articulation = a.articulation;

            if (a.layerScheme.isNotEmpty())
                meta.layerScheme = a.layerScheme;
        }
    }

    return true;
}
