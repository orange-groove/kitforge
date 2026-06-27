import { Box, Flex, Heading, Button, HStack } from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

interface TopBarProps {
  kitName: string;
  editLayout: boolean;
  onToggleEditLayout: () => void;
  onStartLearn: () => void;
  learnActive: boolean;
}

export function TopBar({
  kitName,
  editLayout,
  onToggleEditLayout,
  onStartLearn,
  learnActive,
}: TopBarProps) {
  return (
    <Flex
      px={4}
      py={2}
      align="center"
      justify="space-between"
      bg="kit.panel"
      borderBottom="1px solid"
      borderColor="kit.border"
    >
      <HStack spacing={3}>
        <Heading size="sm" letterSpacing="wide">
          KitForge
        </Heading>
        <Box fontSize="sm" color="kit.textMuted">
          {kitName}
        </Box>
      </HStack>
      <HStack spacing={2}>
        <Button
          size="sm"
          variant="solid"
          colorScheme="blue"
          onClick={onStartLearn}
          isDisabled={learnActive}
        >
          {learnActive ? "Learning…" : "Learn My Kit"}
        </Button>
        <Button
          size="sm"
          variant={editLayout ? "solid" : "outline"}
          colorScheme={editLayout ? "blue" : undefined}
          borderColor="kit.border"
          onClick={onToggleEditLayout}
        >
          Edit Layout
        </Button>
        <Button size="sm" variant="outline" borderColor="kit.border" onClick={() => kitforgeBridge.saveKit()}>
          Save
        </Button>
        <Button size="sm" variant="outline" borderColor="kit.border" onClick={() => kitforgeBridge.loadKit()}>
          Load
        </Button>
      </HStack>
    </Flex>
  );
}
