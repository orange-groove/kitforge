#pragma once

#include "ImportResult.h"
#include "../Models/SampleMetadata.h"
#include "../Models/KitModel.h"

/** Builds KitModel from parsed sample metadata (shared by loose WAV and SFZ importers).

    Pieces are grouped generically (not per library):
      - kick / snare / hi-hat collapse to a single piece, with articulations.
      - toms / crashes / rides / china / splash separate by their file-name "stem"
        so that different drums (Crash 13" vs 16") become distinct pieces while
        articulations of one drum (ride Bow + Bell) merge onto it.
      - rack vs floor toms are split by pitch when the names don't say "floor".
*/
class KitModelBuilder
{
public:
    static KitModel buildFromMetadata (const juce::String& kitName,
                                       const std::vector<SampleMetadata>& samples,
                                       std::vector<ImportWarning>& warnings);

    /** Repairs a loaded kit whose articulations round-robin between several different
        drum bodies (keeps only the dominant voice per articulation). Safe to run on
        any kit; used on load so previously-imported kits get fixed without re-import. */
    static void pruneMixedVoices (KitModel& model);
};
