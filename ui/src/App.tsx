import { useCallback, useEffect, useState } from "react";
import { Box, Flex, Spinner, Text, useDisclosure } from "@chakra-ui/react";
import { AppShell } from "./components/layout/AppShell";
import { LibraryMappingModal } from "./components/library/LibraryMappingModal";
import { SampleSwapModal } from "./components/sample-browser/SampleSwapModal";
import type {
  InstalledKit,
  KitModel,
  LibraryMappingState,
  NativeMessage,
  SampleIndexState,
  SampleSetSummary,
  SwapTarget,
} from "./types/kit";
import {
  isJuceAvailable,
  kitforgeBridge,
  sendReady,
  subscribe,
} from "./bridge/kitforgeBridge";
import { mockCatalog, mockKit } from "./mock/mockKit";

export default function App() {
  const [kit, setKit] = useState<KitModel>(mockKit);
  const [installed, setInstalled] = useState<InstalledKit[]>(mockCatalog.installed);
  const [aiLoading, setAiLoading] = useState(false);
  const [aiMessage, setAiMessage] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [busyLabel, setBusyLabel] = useState<string | null>(null);
  const [mappingState, setMappingState] = useState<LibraryMappingState | null>(null);
  const mappingModal = useDisclosure();

  const [swapTarget, setSwapTarget] = useState<SwapTarget | null>(null);
  const [swapResults, setSwapResults] = useState<SampleSetSummary[]>([]);
  const [sampleIndexState, setSampleIndexState] = useState<SampleIndexState | null>(null);
  const [swapSearching, setSwapSearching] = useState(false);
  const swapModal = useDisclosure();

  const handleMapLibrary = useCallback(
    (kitId: string) => {
      setMappingState(null);
      mappingModal.onOpen();
      kitforgeBridge.getLibraryMapping(kitId);
    },
    [mappingModal],
  );

  const handleSwapSamples = useCallback(
    (target: SwapTarget) => {
      setSwapTarget(target);
      setSwapResults([]);
      setSwapSearching(true);
      swapModal.onOpen();
    },
    [swapModal],
  );

  const handleNativeMessage = useCallback((msg: NativeMessage) => {
    switch (msg.type) {
      case "kitState":
        setKit(msg.kit);
        setAiLoading(false);
        setBusyLabel(null);
        break;
      case "catalogState":
        setInstalled(msg.installed);
        break;
      case "kitInstalled":
        setError(null);
        setBusyLabel(null);
        setAiMessage(msg.message);
        break;
      case "busy":
        setError(null);
        setBusyLabel(msg.label);
        break;
      case "kitRemoved":
        setInstalled((prev) => prev.filter((k) => k.id !== msg.kitId));
        setAiMessage("Kit removed.");
        break;
      case "libraryMappingState":
        setMappingState(msg);
        setError(null);
        break;
      case "libraryApplied":
        setError(null);
        setAiMessage(msg.message);
        setMappingState(null);
        mappingModal.onClose();
        break;
      case "aiBuildComplete":
        setAiLoading(false);
        setAiMessage(msg.message);
        if (msg.success) setError(null);
        break;
      case "sampleSetSearchResults":
        setSwapResults(msg.results);
        setSwapSearching(false);
        break;
      case "sampleIndexState":
        setSampleIndexState({
          sampleSetCount: msg.sampleSetCount,
          libraryCount: msg.libraryCount,
          missingSampleCount: msg.missingSampleCount,
        });
        break;
      case "sampleSwapCompleted":
        setBusyLabel(null);
        setAiMessage("Samples swapped.");
        break;
      case "sampleSwapFailed":
        setBusyLabel(null);
        setError(msg.message);
        break;
      case "validationState":
        if (msg.report.warnings.length > 0 || msg.report.errors.length > 0) {
          setAiMessage(
            [...msg.report.errors, ...msg.report.warnings].slice(0, 3).join(" · "),
          );
        }
        break;
      case "error":
        setError(msg.message);
        setAiLoading(false);
        setBusyLabel(null);
        break;
      case "midiLearnStarted":
        setAiMessage(`Learning MIDI for ${msg.pieceId}…`);
        break;
      case "midiLearnCompleted":
        setAiMessage(`Assigned MIDI note ${msg.midiNote} to ${msg.pieceId}`);
        break;
      case "sampleAssigned":
        setAiMessage(`Assigned ${msg.fileName}`);
        break;
      default:
        break;
    }
  }, [mappingModal]);

  useEffect(() => {
    const unsub = subscribe(handleNativeMessage);
    sendReady();
    return unsub;
  }, [handleNativeMessage]);

  useEffect(() => {
    if (!isJuceAvailable()) {
      setAiMessage("Browser dev mode — using mock kit data.");
    }
  }, []);

  return (
    <Box h="100vh">
      {!isJuceAvailable() && (
        <Box bg="orange.800" px={3} py={1} fontSize="xs" textAlign="center">
          JUCE bridge unavailable — mock data mode
        </Box>
      )}
      {error && (
        <Box bg="red.900" px={3} py={1} fontSize="xs">
          <Text>{error}</Text>
        </Box>
      )}
      <AppShell
        kit={kit}
        installed={installed}
        aiLoading={aiLoading}
        aiMessage={aiMessage}
        onAiGenerate={() => {
          setError(null);
          setAiMessage("Generating kit layout and MIDI mapping…");
          setAiLoading(true);
        }}
        onMapLibrary={handleMapLibrary}
        onSwapSamples={handleSwapSamples}
      />
      <LibraryMappingModal
        state={mappingState}
        isOpen={mappingModal.isOpen}
        onClose={mappingModal.onClose}
      />
      <SampleSwapModal
        isOpen={swapModal.isOpen}
        onClose={swapModal.onClose}
        target={swapTarget}
        results={swapResults}
        indexState={sampleIndexState}
        libraries={installed}
        searching={swapSearching}
      />
      {busyLabel && (
        <Flex
          position="fixed"
          inset={0}
          zIndex={2000}
          bg="blackAlpha.700"
          align="center"
          justify="center"
          direction="column"
          gap={4}
        >
          <Spinner
            thickness="4px"
            speed="0.7s"
            emptyColor="kit.border"
            color="kit.accent"
            size="xl"
          />
          <Text fontSize="sm" color="white">
            {busyLabel}
          </Text>
        </Flex>
      )}
    </Box>
  );
}
