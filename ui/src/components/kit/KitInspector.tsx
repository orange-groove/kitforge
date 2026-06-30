import { useEffect, useState } from "react";
import {
  Box,
  Button,
  Divider,
  Flex,
  Heading,
  Input,
  NumberInput,
  NumberInputField,
  Stack,
  Switch,
  Text,
  Badge,
} from "@chakra-ui/react";
import type { DrumPiece, SwapTarget } from "../../types/kit";
import {
  kitforgeBridge,
  pitchRatioFromSemitones,
  semitonesFromPitchRatio,
} from "../../bridge/kitforgeBridge";
import { PieceMixControls } from "../controls/VolumePanControls";
import { PieceSizeSelect } from "./PieceSizeSelect";
import { LAYOUT_REF_HEIGHT, LAYOUT_REF_WIDTH } from "./layoutUtils";

interface KitInspectorProps {
  piece: DrumPiece | null;
  canvasWidth?: number;
  canvasHeight?: number;
  selectedArticulationName: string | null;
  onSelectArticulation: (articulationId: string, articulationName: string) => void;
  onSwapSamples: (target: SwapTarget) => void;
}

function resolveSelectedArticulation(
  piece: DrumPiece,
  selectedArticulationName: string | null,
) {
  if (!selectedArticulationName) return undefined;

  const target = selectedArticulationName.toLowerCase();
  return piece.articulations.find((art) => art.name.toLowerCase() === target);
}

function isArticulationSelected(
  art: DrumPiece["articulations"][number],
  selectedArticulationName: string | null,
): boolean {
  if (!selectedArticulationName) return false;
  return art.name.toLowerCase() === selectedArticulationName.toLowerCase();
}

export function KitInspector({
  piece,
  canvasWidth = LAYOUT_REF_WIDTH,
  canvasHeight = LAYOUT_REF_HEIGHT,
  selectedArticulationName,
  onSelectArticulation,
  onSwapSamples,
}: KitInspectorProps) {
  const activeArticulation = piece
    ? resolveSelectedArticulation(piece, selectedArticulationName)
    : undefined;

  const [midiDrafts, setMidiDrafts] = useState<Record<string, string>>({});
  const [nameDraft, setNameDraft] = useState("");

  useEffect(() => {
    // Drop in-progress edits when switching pieces so inputs resync to the model.
    setMidiDrafts({});
    setNameDraft(piece?.name ?? "");
  }, [piece?.id, piece?.name]);

  if (!piece) {
    return (
      <Box p={4} h="100%" color="kit.textMuted">
        <Text fontSize="sm">Select a drum piece on the canvas to inspect it.</Text>
      </Box>
    );
  }

  const semitones =
    piece.pitchSemitones ?? semitonesFromPitchRatio(piece.pitch);

  const commitArticulationMidi = (articulationId: string, raw: string) => {
    const n = parseInt(raw, 10);
    if (Number.isNaN(n)) return;
    const clamped = Math.max(0, Math.min(127, n));
    kitforgeBridge.setArticulationMidi(piece.id, articulationId, clamped);
  };

  const commitName = () => {
    const trimmed = nameDraft.trim();
    if (trimmed && trimmed !== piece.name) {
      kitforgeBridge.renamePiece(piece.id, trimmed);
    } else {
      setNameDraft(piece.name);
    }
  };

  return (
    <Box p={4} h="100%" overflowY="auto">
      <Input
        value={nameDraft}
        onChange={(e) => setNameDraft(e.target.value)}
        onBlur={commitName}
        onKeyDown={(e) => {
          if (e.key === "Enter") {
            e.currentTarget.blur();
          } else if (e.key === "Escape") {
            setNameDraft(piece.name);
            e.currentTarget.blur();
          }
        }}
        size="sm"
        fontWeight="bold"
        variant="flushed"
        mb={1}
        px={0}
        borderColor="transparent"
        _hover={{ borderColor: "kit.border" }}
        _focus={{ borderColor: "kit.accent", boxShadow: "none" }}
      />
      <Stack spacing={0.5} fontSize="sm" color="kit.textMuted" mb={4}>
        <Text>Type: {piece.type}</Text>
      </Stack>

      <Box mb={4}>
        <PieceSizeSelect piece={piece} refW={canvasWidth} refH={canvasHeight} />
      </Box>

      <PieceMixControls
        volume={piece.volume}
        pan={piece.pan}
        semitones={semitones}
        onVolumeChange={(volume) => kitforgeBridge.updatePiece(piece.id, { volume })}
        onPanChange={(pan) => kitforgeBridge.updatePiece(piece.id, { pan })}
        onPitchChange={(st) =>
          kitforgeBridge.updatePiece(piece.id, {
            pitch: pitchRatioFromSemitones(st),
          })
        }
      />

      <Flex gap={6} mt={4} align="center" justify="flex-start">
        <Flex align="center" gap={2}>
          <Switch
            size="sm"
            isChecked={piece.muted}
            onChange={(e) =>
              kitforgeBridge.updatePiece(piece.id, { muted: e.target.checked })
            }
          />
          <Text fontSize="sm">Mute</Text>
        </Flex>
        <Flex align="center" gap={2}>
          <Switch
            size="sm"
            isChecked={piece.soloed}
            onChange={(e) =>
              kitforgeBridge.updatePiece(piece.id, { soloed: e.target.checked })
            }
          />
          <Text fontSize="sm">Solo</Text>
        </Flex>
      </Flex>

      <Divider my={4} borderColor="kit.border" />

      <Heading size="xs" mb={2} textTransform="uppercase" letterSpacing="wider">
        Articulations
      </Heading>
      <Stack spacing={2}>
        {piece.articulations.map((art, index) => {
          const hasSample =
            art.hasSample ?? art.layers.some((l) => l.roundRobins.length > 0);
          const selected = isArticulationSelected(art, selectedArticulationName);
          return (
            <Flex
              key={`${art.id}-${art.name}-${index}`}
              justify="space-between"
              align="center"
              p={2}
              bg={selected ? "kit.border" : "kit.bg"}
              border="1px solid"
              borderColor={selected ? "kit.accent" : "transparent"}
              borderRadius="md"
              fontSize="sm"
              cursor="pointer"
              onClick={() => {
                onSelectArticulation(art.id, art.name);
                kitforgeBridge.triggerPiece(piece.id, art.id);
              }}
            >
              <Box>
                <Text fontWeight="medium">{art.name}</Text>
                <Flex
                  align="center"
                  gap={1}
                  mt={1}
                  onClick={(e) => e.stopPropagation()}
                  onPointerDown={(e) => e.stopPropagation()}
                >
                  <Text color="kit.textMuted" fontSize="xs">
                    MIDI
                  </Text>
                  <NumberInput
                    size="xs"
                    min={0}
                    max={127}
                    w="58px"
                    value={midiDrafts[art.id] ?? String(art.midiNote)}
                    onChange={(valueString) => {
                      setMidiDrafts((d) => ({ ...d, [art.id]: valueString }));
                      commitArticulationMidi(art.id, valueString);
                    }}
                    onBlur={() =>
                      setMidiDrafts((d) => {
                        const next = { ...d };
                        delete next[art.id];
                        return next;
                      })
                    }
                    clampValueOnBlur
                  >
                    <NumberInputField px={2} textAlign="center" />
                  </NumberInput>
                </Flex>
              </Box>
              <Badge colorScheme={hasSample ? "green" : "gray"}>
                {hasSample ? "Sample" : "Empty"}
              </Badge>
            </Flex>
          );
        })}
        {piece.articulations.length === 0 && (
          <Text fontSize="sm" color="kit.textMuted">
            No articulations
          </Text>
        )}
      </Stack>

      {activeArticulation && (
        <Stack spacing={2} mt={4}>
          <Button
            size="sm"
            w="100%"
            variant="outline"
            borderColor="kit.border"
            onClick={() => kitforgeBridge.assignSample(piece.id, activeArticulation.id)}
          >
            Assign sample to {activeArticulation.name}
          </Button>
          <Button
            size="sm"
            w="100%"
            colorScheme="blue"
            variant="outline"
            onClick={() =>
              onSwapSamples({
                pieceId: piece.id,
                articulationId: activeArticulation.id,
                instrumentType: piece.type,
                articulationName: activeArticulation.name,
                pieceName: piece.name,
                mode: "articulation",
              })
            }
          >
            Swap samples for {activeArticulation.name}
          </Button>
          <Button
            size="sm"
            w="100%"
            variant="ghost"
            onClick={() => kitforgeBridge.learnMidi(piece.id, activeArticulation.id)}
          >
            Learn MIDI for {activeArticulation.name}
          </Button>
        </Stack>
      )}
    </Box>
  );
}
