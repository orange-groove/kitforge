import { Box, Tab, TabList, TabPanel, TabPanels, Tabs } from "@chakra-ui/react";
import { AIBuilderPanel } from "../ai/AIBuilderPanel";
import { LibraryBrowserPanel } from "../library/LibraryBrowserPanel";
import type { InstalledKit } from "../../types/kit";

interface SidePanelProps {
  tabIndex: number;
  onTabChange: (index: number) => void;
  installed: InstalledKit[];
  aiLoading: boolean;
  aiMessage: string | null;
  onAiGenerate: () => void;
  onMapLibrary: (kitId: string) => void;
}

export function SidePanel({
  tabIndex,
  onTabChange,
  installed,
  aiLoading,
  aiMessage,
  onAiGenerate,
  onMapLibrary,
}: SidePanelProps) {
  return (
    <Box
      w="280px"
      minW="280px"
      h="100%"
      bg="kit.panel"
      borderRight="1px solid"
      borderColor="kit.border"
      display="flex"
      flexDirection="column"
    >
      <Tabs
        index={tabIndex}
        onChange={onTabChange}
        variant="enclosed"
        colorScheme="blue"
        flex="1"
        minH={0}
        display="flex"
        flexDirection="column"
        size="sm"
      >
        <TabList borderColor="kit.border" flexShrink={0}>
          <Tab _selected={{ bg: "kit.bg", borderColor: "kit.border" }}>Kit</Tab>
          <Tab _selected={{ bg: "kit.bg", borderColor: "kit.border" }}>Library</Tab>
          <Tab _selected={{ bg: "kit.bg", borderColor: "kit.border" }}>AI</Tab>
        </TabList>
        <TabPanels flex="1" minH={0} overflow="hidden" display="flex" flexDirection="column">
          <TabPanel p={0} flex="1" minH={0} overflowY="auto">
            <Box p={3} fontSize="sm" color="kit.textMuted">
              Use the canvas to select pieces. Toggle Edit Layout in the top bar to drag pieces.
            </Box>
          </TabPanel>
          <TabPanel p={0} flex="1" minH={0} overflowY="auto">
            <LibraryBrowserPanel installed={installed} onMapLibrary={onMapLibrary} />
          </TabPanel>
          <TabPanel p={0} flex="1" minH={0} overflowY="auto">
            <AIBuilderPanel loading={aiLoading} lastMessage={aiMessage} onGenerate={onAiGenerate} />
          </TabPanel>
        </TabPanels>
      </Tabs>
    </Box>
  );
}
