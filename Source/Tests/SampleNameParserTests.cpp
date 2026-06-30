#include "../Importers/SampleNameParser.h"
#include "../Importers/VendorSampleNaming.h"
#include <vector>

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
            const auto meta = parser.parseFile (juce::File ("ride1Bell_OH_F_1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::ride, "salamander ride bell type");
            ok &= expectEqual (meta.articulation == "Bell", "salamander ride bell art");
            ok &= expectEqual (meta.midiNote == 53, "salamander ride bell midi");
        }

        {
            const auto meta = parser.parseFile (juce::File ("ride1_OH_FF_1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::ride, "salamander ride bow type");
            ok &= expectEqual (meta.articulation == "Bow", "salamander ride bow art");
            ok &= expectEqual (meta.midiNote == 51, "salamander ride bow midi");
        }

        {
            const auto meta = parser.parseFile (juce::File ("china1_OH_FF_1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::china, "salamander china type");
            ok &= expectEqual (meta.articulation == "Center", "salamander china art");
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

        {
            std::vector<SampleMetadata> niSamples;
            SampleMetadata closed;
            closed.filePath = "hihat - closed - 1.wav";
            closed.instrumentType = DrumPieceType::hiHat;
            closed.pieceGroupKey = "hihat";
            closed.articulation = "Closed";
            closed.midiNote = 42;
            closed.layerIndex = 1;
            niSamples.push_back (closed);

            for (int layer = 2; layer <= 8; ++layer)
            {
                SampleMetadata m = closed;
                m.filePath = "hihat - closed - " + juce::String (layer) + ".wav";
                m.layerIndex = layer;
                niSamples.push_back (std::move (m));
            }

            VendorSampleNaming::resolveLayerAssignments (niSamples);
            ok &= expectEqual (niSamples[0].minVelocity == 1 && niSamples[0].maxVelocity <= 16, "ni hat vel layer 1");
            ok &= expectEqual (niSamples[7].maxVelocity == 127, "ni hat vel layer 8 top");
            ok &= expectEqual (niSamples[0].roundRobinIndex == 0, "ni hat single sample per layer");
        }

        {
            const auto meta = parser.parseFile (juce::File ("hihat - closed - 1.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::hiHat, "ni hihat type");
            ok &= expectEqual (meta.articulation == "Closed", "ni hihat art");
            ok &= expectEqual (meta.layerIndex == 1, "ni hihat layer");
            ok &= expectEqual (meta.pieceGroupKey == "hihat", "ni hihat piece");
        }

        {
            const auto meta = parser.parseFile (juce::File ("bop kick - snares off - 2.wav"));
            ok &= expectEqual (meta.instrumentType == DrumPieceType::kick, "ni bop kick type");
            ok &= expectEqual (meta.pieceGroupKey == "bop_kick", "ni bop kick piece");
            ok &= expectEqual (meta.layerIndex == 2, "ni bop kick layer");
        }

        return ok;
    }
}
