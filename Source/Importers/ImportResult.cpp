#include "ImportResult.h"

ImportPreviewStats ImportResult::computeStats (const KitModel& model, int wavFileCount)
{
    ImportPreviewStats stats;
    stats.wavFileCount = wavFileCount;
    stats.pieceCount = (int) model.getPieces().size();

    for (const auto& piece : model.getPieces())
    {
        stats.articulationCount += (int) piece.articulations.size();

        for (const auto& art : piece.articulations)
        {
            stats.layerCount += (int) art.layers.size();

            for (const auto& layer : art.layers)
                stats.roundRobinCount += layer.roundRobins.size();
        }
    }

    return stats;
}
