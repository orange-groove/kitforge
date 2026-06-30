import type { DrumPieceType } from "../../types/kit";

/** Standard drum/cymbal diameters supported when adding pieces. */
export const PIECE_DIAMETERS_IN = [8, 10, 12, 14, 16, 18, 20, 22, 24] as const;

export type PieceDiameterIn = (typeof PIECE_DIAMETERS_IN)[number];

export interface AddPieceOption {
  type: DrumPieceType;
  label: string;
}

export const DRUM_ADD_OPTIONS: AddPieceOption[] = [
  { type: "kick", label: "Kick" },
  { type: "snare", label: "Snare" },
  { type: "rackTom", label: "Rack Tom" },
  { type: "floorTom", label: "Floor Tom" },
];

export const CYMBAL_ADD_OPTIONS: AddPieceOption[] = [
  { type: "hiHat", label: "Hi-Hat" },
  { type: "crash", label: "Crash" },
  { type: "ride", label: "Ride" },
  { type: "china", label: "China" },
  { type: "splash", label: "Splash" },
];

/** 6 layout pixels per inch on the 980×680 reference canvas (matches C++). */
export const LAYOUT_PIXELS_PER_INCH = 6;

export function pieceDiameterPixels(diameterInches: number): number {
  return Math.min(200, Math.max(8, diameterInches * LAYOUT_PIXELS_PER_INCH));
}

/** Diameter in inches from normalized layout width/height (UI kit state). */
export function pieceDiameterInchesFromNormalized(
  widthNorm: number,
  heightNorm: number,
  refW: number,
  refH: number,
): number {
  const diameterPx = Math.max(widthNorm * refW, heightNorm * refH);
  return diameterPx / LAYOUT_PIXELS_PER_INCH;
}

/** Snap a measured diameter to the nearest allowed inch size. */
export function nearestPieceDiameterInches(inches: number): PieceDiameterIn {
  let best: PieceDiameterIn = PIECE_DIAMETERS_IN[0];
  let bestDist = Math.abs(inches - best);
  for (const d of PIECE_DIAMETERS_IN) {
    const dist = Math.abs(inches - d);
    if (dist < bestDist) {
      best = d;
      bestDist = dist;
    }
  }
  return best;
}

/** Normalized width/height for a circular piece at the given inch diameter. */
export function normalizedSizeFromDiameterInches(
  diameterInches: number,
  refW: number,
  refH: number,
): { width: number; height: number } {
  const px = pieceDiameterPixels(diameterInches);
  return { width: px / refW, height: px / refH };
}
