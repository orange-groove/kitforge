import { useEffect, useState } from "react";
import { Box, Flex } from "@chakra-ui/react";
import { TopBar } from "./TopBar";
import { SidePanel } from "./SidePanel";
import { DrumCanvas } from "../kit/DrumCanvas";
import { KitInspector } from "../kit/KitInspector";
import { LearnKitBanner } from "../kit/LearnKitBanner";
import { useLearnKitWizard } from "../kit/useLearnKitWizard";
import type { DrumPiece, InstalledKit, KitModel, SwapTarget } from "../../types/kit";

interface AppShellProps {
  kit: KitModel;
  installed: InstalledKit[];
  aiLoading: boolean;
  aiMessage: string | null;
  onAiGenerate: () => void;
  onMapLibrary: (kitId: string) => void;
  onSwapSamples: (target: SwapTarget) => void;
}

export function AppShell({
  kit,
  installed,
  aiLoading,
  aiMessage,
  onAiGenerate,
  onMapLibrary,
  onSwapSamples,
}: AppShellProps) {
  const [tabIndex, setTabIndex] = useState(0);
  const [editLayout, setEditLayout] = useState(false);
  const learn = useLearnKitWizard(kit);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [selectedArticulationId, setSelectedArticulationId] = useState<string | null>(null);
  const [selectedArticulationName, setSelectedArticulationName] = useState<string | null>(
    null,
  );

  const selectedPiece: DrumPiece | null =
    kit.pieces.find((p) => p.id === selectedId) ?? null;

  useEffect(() => {
    if (!selectedId || !selectedArticulationName) return;

    const piece = kit.pieces.find((p) => p.id === selectedId);
    if (!piece) {
      setSelectedId(null);
      setSelectedArticulationId(null);
      setSelectedArticulationName(null);
      return;
    }

    const target = selectedArticulationName.toLowerCase();
    const art = piece.articulations.find((a) => a.name.toLowerCase() === target);
    if (art && art.id !== selectedArticulationId) {
      setSelectedArticulationId(art.id);
    }
  }, [kit, selectedId, selectedArticulationId, selectedArticulationName]);

  return (
    <Flex direction="column" h="100vh" bg="kit.bg">
      <TopBar
        kitName={kit.kitName}
        editLayout={editLayout}
        onToggleEditLayout={() => setEditLayout((v) => !v)}
        onStartLearn={learn.start}
        learnActive={learn.active}
      />
      <Flex flex="1" minH={0}>
        <SidePanel
          tabIndex={tabIndex}
          onTabChange={setTabIndex}
          installed={installed}
          aiLoading={aiLoading}
          aiMessage={aiMessage}
          onAiGenerate={onAiGenerate}
          onMapLibrary={onMapLibrary}
        />
        <Box flex="1" p={3} display="flex" minW={0} position="relative">
          <DrumCanvas
            kit={kit}
            selectedPieceId={selectedId}
            selectedArticulationId={selectedArticulationId}
            editLayout={editLayout}
            onSwapSamples={onSwapSamples}
            learnActivePieceId={learn.activePieceId}
            learnDonePieceIds={learn.donePieceIds}
            onSelectPiece={(pieceId, articulationId) => {
              setSelectedId(pieceId);
              if (pieceId == null) {
                setSelectedArticulationId(null);
                setSelectedArticulationName(null);
                return;
              }
              if (articulationId == null) {
                setSelectedArticulationId(null);
                setSelectedArticulationName(null);
                return;
              }
              const piece = kit.pieces.find((p) => p.id === pieceId);
              const art = piece?.articulations.find((a) => a.id === articulationId);
              setSelectedArticulationId(articulationId);
              setSelectedArticulationName(art?.name ?? null);
            }}
          />
          <LearnKitBanner wizard={learn} />
        </Box>
        <Box
          w="260px"
          minW="260px"
          bg="kit.panel"
          borderLeft="1px solid"
          borderColor="kit.border"
        >
          <KitInspector
            piece={selectedPiece}
            selectedArticulationId={selectedArticulationId}
            onSelectArticulation={setSelectedArticulationId}
            onSwapSamples={onSwapSamples}
          />
        </Box>
      </Flex>
    </Flex>
  );
}
