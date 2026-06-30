#pragma once

#include "DrumPieceTypes.h"

class KitModel;
struct DrumPiece;

/** Places a newly added tom in the kit layout row (rack or floor). May reposition siblings. */
void applyTomLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                float canvasWidth, float canvasHeight);

/** Places hi-hats along a diagonal from the leftmost kick's top-right corner. */
void applyHiHatLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                  float canvasWidth, float canvasHeight);

/** Places rides along the same down-right diagonal as floor toms. */
void applyRideLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                 float canvasWidth, float canvasHeight);
