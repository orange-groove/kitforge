import {
  Box,
  Button,
  Checkbox,
  Flex,
  Modal,
  ModalBody,
  ModalContent,
  ModalFooter,
  ModalHeader,
  ModalOverlay,
  Select,
  Stack,
  Text,
} from "@chakra-ui/react";
import { useCallback, useEffect, useState } from "react";
import type { LibraryMapping, LibraryMappingState } from "../../types/kit";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

interface LibraryMappingModalProps {
  state: LibraryMappingState | null;
  isOpen: boolean;
  onClose: () => void;
}

export function LibraryMappingModal({ state, isOpen, onClose }: LibraryMappingModalProps) {
  const [mappings, setMappings] = useState<LibraryMapping[]>([]);

  useEffect(() => {
    if (state) {
      setMappings(state.mappings.map((m) => ({ ...m })));
    }
  }, [state]);

  const updateMapping = useCallback(
    (sourceKey: string, patch: Partial<LibraryMapping>) => {
      setMappings((prev) =>
        prev.map((m) => (m.sourceKey === sourceKey ? { ...m, ...patch } : m)),
      );
    },
    [],
  );

  const applySuggested = useCallback(() => {
    if (state) {
      setMappings(state.mappings.map((m) => ({ ...m })));
    }
  }, [state]);

  const handleApply = useCallback(() => {
    if (!state) return;
    kitforgeBridge.applyLibraryMapping(state.libraryId, mappings);
    onClose();
  }, [state, mappings, onClose]);

  if (!state) {
    return (
      <Modal isOpen={isOpen} onClose={onClose} size="md" isCentered>
        <ModalOverlay />
        <ModalContent bg="kit.panel">
          <ModalBody py={8}>
            <Text textAlign="center" color="kit.textMuted">
              Loading library mapping…
            </Text>
          </ModalBody>
        </ModalContent>
      </Modal>
    );
  }

  const sourceByKey = new Map(state.sources.map((s) => [s.key, s]));

  return (
    <Modal isOpen={isOpen} onClose={onClose} size="4xl" scrollBehavior="inside">
      <ModalOverlay />
      <ModalContent bg="kit.panel" maxH="85vh">
        <ModalHeader fontSize="md" pb={1}>
          Map Library to Kit
        </ModalHeader>
        <ModalBody>
          <Text fontSize="sm" color="kit.textMuted" mb={3}>
            Library: {state.libraryName} → Kit: {state.kitName} ({state.targets.length}{" "}
            articulation slots)
          </Text>
          <Stack spacing={2}>
            {mappings.map((mapping) => {
              const source = sourceByKey.get(mapping.sourceKey);
              if (!source) return null;

              return (
                <Flex
                  key={mapping.sourceKey}
                  gap={2}
                  align="center"
                  p={2}
                  bg="kit.bg"
                  borderRadius="md"
                  border="1px solid"
                  borderColor="kit.border"
                  fontSize="sm"
                >
                  <Checkbox
                    isChecked={mapping.enabled}
                    onChange={(e) =>
                      updateMapping(mapping.sourceKey, { enabled: e.target.checked })
                    }
                  />
                  <Box flex="1" minW={0}>
                    <Text fontWeight="medium" noOfLines={1}>
                      {source.pieceName} / {source.articulationName}
                    </Text>
                    <Text fontSize="xs" color="kit.textMuted">
                      MIDI {source.midiNote} · {source.sampleCount} samples
                    </Text>
                  </Box>
                  <Select
                    size="sm"
                    w="240px"
                    flexShrink={0}
                    value={mapping.targetKey || ""}
                    onChange={(e) =>
                      updateMapping(mapping.sourceKey, {
                        targetKey: e.target.value,
                      })
                    }
                    isDisabled={!mapping.enabled}
                  >
                    <option value="">(Unmapped)</option>
                    {state.targets.map((target) => (
                      <option key={target.key} value={target.key}>
                        {target.label}
                      </option>
                    ))}
                  </Select>
                </Flex>
              );
            })}
            {mappings.length === 0 && (
              <Text fontSize="sm" color="kit.textMuted">
                No library articulations found.
              </Text>
            )}
          </Stack>
        </ModalBody>
        <ModalFooter gap={2}>
          <Button variant="ghost" onClick={onClose}>
            Cancel
          </Button>
          <Button variant="outline" borderColor="kit.border" onClick={applySuggested}>
            Auto-map
          </Button>
          <Button colorScheme="blue" onClick={handleApply}>
            Apply mappings
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}
