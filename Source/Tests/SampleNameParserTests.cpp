#include "../Importers/SampleNameParser.h"

namespace SampleNameParserSelfTests
{
    static bool expectEqual (bool condition, const char* message)
    {
        jassertquiet (condition || message == nullptr);
        juce::ignoreUnused (message);
        return condition;
    }

    /** Debug helper — returns false if any filename parse check fails. */
    bool runAll()
    {
        SampleNameParser parser;
        bool ok = true;

        {
            const auto meta = parser.parseFile (juce::File ("Kick_Center_V127_RR1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::kick, "kick type");
            ok &= expectEqual (meta.articulation == "Center", "kick articulation");
            ok &= expectEqual (meta.midiNote == 36, "kick midi");
            ok &= expectEqual (meta.roundRobinIndex == 1, "kick rr");
            ok &= expectEqual (meta.confidence >= 0.9f, "kick confidence");
        }

        {
            const auto meta = parser.parseFile (juce::File ("Snare_Rimshot_V100_RR2.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::snare, "snare type");
            ok &= expectEqual (meta.articulation == "Rimshot", "snare articulation");
            ok &= expectEqual (meta.midiNote == 40, "snare rim midi");
            ok &= expectEqual (meta.roundRobinIndex == 2, "snare rr");
        }

        {
            const auto meta = parser.parseFile (juce::File ("Hat_Closed_V070_RR1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::hiHat, "hat type");
            ok &= expectEqual (meta.articulation == "Closed", "hat articulation");
            ok &= expectEqual (meta.midiNote == 42, "hat midi");
            ok &= expectEqual (meta.chokeGroupId == "hihat", "hat choke");
        }

        {
            const auto meta = parser.parseFile (juce::File ("Ride_Bell_V120_RR3.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::ride, "ride type");
            ok &= expectEqual (meta.articulation == "Bell", "ride bell");
            ok &= expectEqual (meta.midiNote == 53, "ride bell midi");
            ok &= expectEqual (meta.roundRobinIndex == 3, "ride rr");
        }

        {
            const auto meta = parser.parseFile (juce::File ("FloorTom2_Center_hard_rr2.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::floorTom, "floor tom type");
            ok &= expectEqual (meta.instrumentIndex == 1, "floor tom index");
            ok &= expectEqual (meta.minVelocity == 91 && meta.maxVelocity == 127, "floor tom hard vel");
            ok &= expectEqual (meta.roundRobinIndex == 2, "floor tom rr");
        }

        {
            const auto meta = parser.parseFile (juce::File ("Crash1_Edge_soft_rr1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::crash, "crash type");
            ok &= expectEqual (meta.articulation == "Edge", "crash edge");
            ok &= expectEqual (meta.minVelocity == 1 && meta.maxVelocity == 45, "crash soft vel");
            ok &= expectEqual (meta.roundRobinIndex == 1, "crash rr");
        }

        return ok;
    }
}
