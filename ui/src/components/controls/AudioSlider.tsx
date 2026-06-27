import { Slider } from "@cutoff/audio-ui-react";

export interface AudioSliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  onChange: (value: number) => void;
}

export function AudioSlider({
  label,
  value,
  min,
  max,
  onChange,
}: AudioSliderProps) {
  return (
    <Slider
      label={label}
      value={value}
      min={min}
      max={max}
      orientation="vertical"
      onChange={(e) => onChange(e.value)}
    />
  );
}
