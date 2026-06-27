import { Input, Select, SimpleGrid } from "@chakra-ui/react";
import type { DrumPieceType, InstalledKit } from "../../types/kit";

export interface SampleSetFilterValues {
  text: string;
  libraryId: string;
  instrumentType: string;
  articulation: string;
}

const INSTRUMENT_TYPES: DrumPieceType[] = [
  "kick",
  "snare",
  "rackTom",
  "floorTom",
  "hiHat",
  "crash",
  "ride",
  "china",
  "splash",
  "accessory",
];

interface SampleSetFiltersProps {
  values: SampleSetFilterValues;
  libraries: InstalledKit[];
  onChange: (next: SampleSetFilterValues) => void;
}

export function SampleSetFilters({ values, libraries, onChange }: SampleSetFiltersProps) {
  const set = (patch: Partial<SampleSetFilterValues>) => onChange({ ...values, ...patch });

  return (
    <SimpleGrid columns={{ base: 1, md: 2 }} spacing={2}>
      <Input
        size="sm"
        placeholder="Search (e.g. punchy metal)"
        bg="kit.bg"
        borderColor="kit.border"
        value={values.text}
        onChange={(e) => set({ text: e.target.value })}
      />
      <Input
        size="sm"
        placeholder="Articulation (e.g. center)"
        bg="kit.bg"
        borderColor="kit.border"
        value={values.articulation}
        onChange={(e) => set({ articulation: e.target.value })}
      />
      <Select
        size="sm"
        bg="kit.bg"
        borderColor="kit.border"
        value={values.instrumentType}
        onChange={(e) => set({ instrumentType: e.target.value })}
      >
        <option value="">Any instrument</option>
        {INSTRUMENT_TYPES.map((t) => (
          <option key={t} value={t}>
            {t}
          </option>
        ))}
      </Select>
      <Select
        size="sm"
        bg="kit.bg"
        borderColor="kit.border"
        value={values.libraryId}
        onChange={(e) => set({ libraryId: e.target.value })}
      >
        <option value="">All libraries</option>
        {libraries.map((lib) => (
          <option key={lib.id} value={lib.id}>
            {lib.name}
          </option>
        ))}
      </Select>
    </SimpleGrid>
  );
}
