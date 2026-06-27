import { Box, SimpleGrid, Text } from "@chakra-ui/react";
import { AudioKnob } from "./AudioKnob";

interface PieceMixControlsProps {
  volume: number;
  pan: number;
  semitones: number;
  onVolumeChange: (v: number) => void;
  onPanChange: (v: number) => void;
  onPitchChange: (semitones: number) => void;
}

export function PieceMixControls({
  volume,
  pan,
  semitones,
  onVolumeChange,
  onPanChange,
  onPitchChange,
}: PieceMixControlsProps) {
  return (
    <Box
      className="dark"
      bg="kit.bg"
      border="1px solid"
      borderColor="kit.border"
      borderRadius="md"
      p={3}
    >
      <Text
        fontSize="xs"
        fontWeight="semibold"
        textTransform="uppercase"
        letterSpacing="wider"
        color="kit.textMuted"
        mb={3}
      >
        Mix
      </Text>
      <SimpleGrid columns={3} spacing={1} justifyItems="center">
        <AudioKnob
          label="Vol"
          value={volume * 100}
          min={0}
          max={200}
          unit="%"
          onChange={(v) => onVolumeChange(v / 100)}
        />
        <AudioKnob
          label="Pan"
          value={pan * 100}
          min={-100}
          max={100}
          bipolar
          valueFormatter={(v) => `${v >= 0 ? "+" : ""}${Math.round(v)}`}
          onChange={(v) => onPanChange(v / 100)}
        />
        <AudioKnob
          label="Pitch"
          value={semitones}
          min={-24}
          max={24}
          unit="st"
          bipolar
          valueFormatter={(v) => `${v >= 0 ? "+" : ""}${v.toFixed(1)}`}
          onChange={onPitchChange}
        />
      </SimpleGrid>
    </Box>
  );
}
