import { Box } from "@chakra-ui/react";
import { Knob } from "@cutoff/audio-ui-react";

export interface AudioKnobProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  bipolar?: boolean;
  valueFormatter?: (value: number) => string;
  onChange: (value: number) => void;
}

export function AudioKnob({
  label,
  value,
  min,
  max,
  unit,
  bipolar,
  valueFormatter,
  onChange,
}: AudioKnobProps) {
  return (
    <Box className="dark" color="#e8e8e8">
      <Knob
        label={label}
        value={value}
        min={min}
        max={max}
        unit={unit}
        bipolar={bipolar}
        size="small"
        variant="abstract"
        thickness={0.55}
        color="#5b8def"
        valueAsLabel="interactive"
        valueFormatter={
          valueFormatter
            ? (v) => valueFormatter(v)
            : undefined
        }
        onChange={(e) => onChange(e.value)}
      />
    </Box>
  );
}
