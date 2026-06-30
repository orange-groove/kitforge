import { FormControl, FormLabel, Select } from "@chakra-ui/react";
import type { DrumPiece } from "../../types/kit";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";
import {
  nearestPieceDiameterInches,
  normalizedSizeFromDiameterInches,
  pieceDiameterInchesFromNormalized,
  PIECE_DIAMETERS_IN,
  type PieceDiameterIn,
} from "./addPieceCatalog";
import { LAYOUT_REF_HEIGHT, LAYOUT_REF_WIDTH } from "./layoutUtils";

interface PieceSizeSelectProps {
  piece: DrumPiece;
  refW?: number;
  refH?: number;
}

export function PieceSizeSelect({
  piece,
  refW = LAYOUT_REF_WIDTH,
  refH = LAYOUT_REF_HEIGHT,
}: PieceSizeSelectProps) {
  if (piece.type === "accessory") return null;

  const measuredIn = pieceDiameterInchesFromNormalized(
    piece.width,
    piece.height,
    refW,
    refH,
  );
  const selectedIn = nearestPieceDiameterInches(measuredIn);

  const onChange = (inches: PieceDiameterIn) => {
    const { width, height } = normalizedSizeFromDiameterInches(inches, refW, refH);
    kitforgeBridge.resizePiece(piece.id, width, height);
  };

  return (
    <FormControl size="sm">
      <FormLabel fontSize="sm" mb={1} color="kit.textMuted">
        Size
      </FormLabel>
      <Select
        size="sm"
        value={String(selectedIn)}
        onChange={(e) => onChange(Number(e.target.value) as PieceDiameterIn)}
      >
        {PIECE_DIAMETERS_IN.map((d) => (
          <option key={d} value={d}>
            {d}&Prime;
          </option>
        ))}
      </Select>
    </FormControl>
  );
}
