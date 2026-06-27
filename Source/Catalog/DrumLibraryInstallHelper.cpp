#include "DrumLibraryInstallHelper.h"

void DrumLibraryInstallHelper::anchorKitSamplePaths (KitModel& kit, const juce::File& kitForgeDir)
{
    juce::StringArray pieceIds;

    for (const auto& piece : kit.getPieces())
        pieceIds.add (piece.id);

    for (const auto& pieceId : pieceIds)
    {
        auto* piece = kit.findPieceById (pieceId);

        if (piece == nullptr)
            continue;

        for (auto& art : piece->articulations)
        {
            for (auto& layer : art.layers)
            {
                for (auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isEmpty())
                        continue;

                    const auto normalized = sample.filePath.replaceCharacter ('\\', '/');
                    juce::File resolved (normalized);

                    if (! juce::File::isAbsolutePath (normalized))
                        resolved = kitForgeDir.getParentDirectory().getChildFile ("source").getChildFile (normalized);

                    if (! resolved.existsAsFile())
                        resolved = juce::File (normalized);

                    if (resolved.existsAsFile())
                        sample.filePath = resolved.getRelativePathFrom (kitForgeDir);
                }
            }
        }
    }
}
