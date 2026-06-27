import { useEffect, useState } from "react";
import {
  Box,
  Button,
  Divider,
  Flex,
  Heading,
  NumberDecrementStepper,
  NumberIncrementStepper,
  NumberInput,
  NumberInputField,
  NumberInputStepper,
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
import { findArticulationByName, isRidePiece, rideEdgeArticulation } from "./layoutUtils";

interface KitInspectorProps {
  piece: DrumPiece | null;
  selectedArticulationId: string | null;
  onSelectArticulation: (articulationId: string | null) => void;
  onSwapSamples: (target: SwapTarget) => void;
}

export function KitInspector({
  piece,
  selectedArticulationId,
  onSelectArticulation,
  onSwapSamples,
}: KitInspectorProps) {
  const activeArticulation = piece
    ? piece.articulations.find((art) => art.id === selectedArticulationId) ??
      (isRidePiece(piece) ? rideEdgeArticulation(piece) : undefined) ??
      piece.articulations[0]
    : undefined;

  const [midiDraft, setMidiDraft] = useState("");

  useEffect(() => {
    setMidiDraft(activeArticulation ? String(activeArticulation.midiNote) : "");
    // Resync only when the selected piece/articulation changes (not on every echo).
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [piece?.id, activeArticulation?.id, activeArticulation?.midiNote]);

  if (!piece) {
    return (
      <Box p={4} h="100%" color="kit.textMuted">
        <Text fontSize="sm">Select a drum piece on the canvas to inspect it.</Text>
      </Box>
    );
  }

  const semitones =
    piece.pitchSemitones ?? semitonesFromPitchRatio(piece.pitch);

  const commitMidiNote = (raw: string) => {
    const n = parseInt(raw, 10);
    if (Number.isNaN(n) || !activeArticulation) return;
    const clamped = Math.max(0, Math.min(127, n));
    kitforgeBridge.setArticulationMidi(piece.id, activeArticulation.id, clamped);
  };

  return (
    <Box p={4} h="100%" overflowY="auto">
      <Heading size="sm" mb={1}>
        {piece.name}
      </Heading>
      <Stack spacing={0.5} fontSize="sm" color="kit.textMuted" mb={4}>
        <Text>Type: {piece.type}</Text>
        {isRidePiece(piece) ? (
          <Text>Edge MIDI: {rideEdgeArticulation(piece)?.midiNote ?? piece.primaryMidiNote}</Text>
        ) : (
          <Text>Primary MIDI: {piece.primaryMidiNote}</Text>
        )}
        {isRidePiece(piece) && findArticulationByName(piece, "Bell") && (
          <Text>Bell MIDI: {findArticulationByName(piece, "Bell")!.midiNote}</Text>
        )}
      </Stack>

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
        {piece.articulations.map((art) => {
          const hasSample =
            art.hasSample ?? art.layers.some((l) => l.roundRobins.length > 0);
          const selected = art.id === activeArticulation?.id;
          return (
            <Flex
              key={art.id}
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
                onSelectArticulation(art.id);
                kitforgeBridge.triggerPiece(piece.id, art.id);
              }}
            >
              <Box>
                <Text fontWeight="medium">{art.name}</Text>
                <Text color="kit.textMuted" fontSize="xs">
                  MIDI {art.midiNote}
                </Text>
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
          <Flex align="center" justify="space-between" gap={2}>
            <Text fontSize="sm">Trigger MIDI note</Text>
            <NumberInput
              size="sm"
              min={0}
              max={127}
              w="96px"
              value={midiDraft}
              onChange={(valueString) => {
                setMidiDraft(valueString);
                commitMidiNote(valueString);
              }}
              clampValueOnBlur
            >
              <NumberInputField />
              <NumberInputStepper>
                <NumberIncrementStepper />
                <NumberDecrementStepper />
              </NumberInputStepper>
            </NumberInput>
          </Flex>
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
