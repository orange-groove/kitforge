import { useEffect, useMemo, useState } from "react";
import {
  Box,
  Flex,
  Modal,
  ModalBody,
  ModalCloseButton,
  ModalContent,
  ModalFooter,
  ModalHeader,
  ModalOverlay,
  Button,
  Select,
  SimpleGrid,
  Spinner,
  Text,
} from "@chakra-ui/react";
import type {
  InstalledKit,
  SampleIndexState,
  SampleSetSummary,
  SwapMode,
  SwapTarget,
} from "../../types/kit";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";
import { SampleSetFilters, type SampleSetFilterValues } from "./SampleSetFilters";
import { SampleSetCard } from "./SampleSetCard";

interface SampleSwapModalProps {
  isOpen: boolean;
  onClose: () => void;
  target: SwapTarget | null;
  results: SampleSetSummary[];
  indexState: SampleIndexState | null;
  libraries: InstalledKit[];
  searching: boolean;
}

const emptyFilters: SampleSetFilterValues = {
  text: "",
  libraryId: "",
  instrumentType: "",
  articulation: "",
};

export function SampleSwapModal({
  isOpen,
  onClose,
  target,
  results,
  indexState,
  libraries,
  searching,
}: SampleSwapModalProps) {
  const [filters, setFilters] = useState<SampleSetFilterValues>(emptyFilters);
  const [mode, setMode] = useState<SwapMode>("articulation");

  // Seed filters from the swap target whenever the modal opens for a new target.
  useEffect(() => {
    if (!isOpen || !target) return;
    setMode(target.mode === "layer" ? "layer" : target.mode);
    setFilters({
      text: "",
      libraryId: "",
      instrumentType: target.instrumentType ?? "",
      articulation: target.articulationName ?? "",
    });
  }, [isOpen, target]);

  // Debounced search whenever filters change while the modal is open.
  useEffect(() => {
    if (!isOpen) return;
    const handle = window.setTimeout(() => {
      kitforgeBridge.searchSampleSets({
        instrumentType: filters.instrumentType || undefined,
        articulation: filters.articulation || undefined,
        libraryId: filters.libraryId || undefined,
        text: filters.text || undefined,
      });
    }, 250);
    return () => window.clearTimeout(handle);
  }, [isOpen, filters]);

  const onUse = (set: SampleSetSummary) => {
    if (!target) return;
    kitforgeBridge.swapSampleSet(
      {
        pieceId: target.pieceId,
        articulationId: target.articulationId,
        layerId: target.layerId,
        mode,
      },
      set.id,
    );
    onClose();
  };

  const subtitle = useMemo(() => {
    if (!target) return "";
    const piece = target.pieceName ?? target.instrumentType;
    return mode === "piece"
      ? `Whole piece · ${piece}`
      : `${piece}${target.articulationName ? " · " + target.articulationName : ""}`;
  }, [target, mode]);

  return (
    <Modal isOpen={isOpen} onClose={onClose} size="4xl" scrollBehavior="inside">
      <ModalOverlay />
      <ModalContent bg="kit.panel" maxH="85vh">
        <ModalHeader fontSize="md" pb={1}>
          Swap Samples
          <Text fontSize="xs" color="kit.textMuted" fontWeight="normal">
            {subtitle}
          </Text>
        </ModalHeader>
        <ModalCloseButton />
        <ModalBody>
          <Flex justify="space-between" align="center" mb={2} gap={2}>
            <Select
              size="sm"
              w="220px"
              bg="kit.bg"
              borderColor="kit.border"
              value={mode}
              onChange={(e) => setMode(e.target.value as SwapMode)}
            >
              <option value="articulation">Replace this articulation</option>
              <option value="piece">Replace whole piece</option>
              {target?.layerId && <option value="layer">Replace this velocity layer</option>}
            </Select>
            <Text fontSize="xs" color="kit.textMuted">
              {indexState
                ? `${indexState.sampleSetCount} sets · ${indexState.libraryCount} libraries`
                : "Indexing…"}
            </Text>
          </Flex>

          <SampleSetFilters values={filters} libraries={libraries} onChange={setFilters} />

          <Box mt={3}>
            {searching && results.length === 0 ? (
              <Flex justify="center" py={8}>
                <Spinner color="kit.accent" />
              </Flex>
            ) : results.length === 0 ? (
              <Text fontSize="sm" color="kit.textMuted" py={8} textAlign="center">
                No compatible sample sets found. Import more libraries or widen the filters.
              </Text>
            ) : (
              <SimpleGrid columns={{ base: 1, md: 2 }} spacing={2}>
                {results.map((set) => (
                  <SampleSetCard key={set.id} set={set} onUse={onUse} />
                ))}
              </SimpleGrid>
            )}
          </Box>
        </ModalBody>
        <ModalFooter gap={2}>
          <Button variant="ghost" onClick={onClose}>
            Cancel
          </Button>
          <Button
            variant="outline"
            borderColor="kit.border"
            onClick={() => kitforgeBridge.rebuildSampleIndex()}
          >
            Rebuild index
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}
