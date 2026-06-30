import {
  Box,
  Flex,
  Heading,
  Button,
  HStack,
  Menu,
  MenuButton,
  MenuList,
  MenuItem,
  MenuDivider,
} from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";
import { AddPieceMenuItems } from "../kit/AddPieceMenu";

interface TopBarProps {
  kitName: string;
  editLayout: boolean;
  canUndo: boolean;
  canRedo: boolean;
  onToggleEditLayout: () => void;
  onStartLearn: () => void;
  learnActive: boolean;
}

export function TopBar({
  kitName,
  editLayout,
  canUndo,
  canRedo,
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
        <Menu>
          <MenuButton
            as={Button}
            size="sm"
            variant="ghost"
            colorScheme="gray"
            fontWeight="normal"
          >
            File
          </MenuButton>
          <MenuList minW="180px">
            <MenuItem command="⌘S" onClick={() => kitforgeBridge.saveKit()}>
              Save
            </MenuItem>
            <MenuItem onClick={() => kitforgeBridge.saveKitAs()}>
              Save As…
            </MenuItem>
            <MenuItem onClick={() => kitforgeBridge.loadKit()}>Load…</MenuItem>
            <MenuDivider borderColor="kit.border" />
            <MenuItem
              command="⌘Z"
              isDisabled={!canUndo}
              onClick={() => kitforgeBridge.undo()}
            >
              Undo
            </MenuItem>
            <MenuItem
              command="⇧⌘Z"
              isDisabled={!canRedo}
              onClick={() => kitforgeBridge.redo()}
            >
              Redo
            </MenuItem>
          </MenuList>
        </Menu>
        <Menu>
          <MenuButton
            as={Button}
            size="sm"
            variant="ghost"
            colorScheme="gray"
            fontWeight="normal"
          >
            Kit
          </MenuButton>
          <MenuList minW="200px">
            <Menu placement="right-start" isLazy gutter={4}>
              <MenuButton
                as={MenuItem}
                closeOnSelect={false}
                _hover={{ bg: "kit.border" }}
                _focus={{ bg: "kit.border" }}
              >
                Add Piece…
              </MenuButton>
              <MenuList minW="200px" maxH="70vh" overflowY="auto" py={2}>
                <AddPieceMenuItems />
              </MenuList>
            </Menu>
          </MenuList>
        </Menu>
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
      </HStack>
    </Flex>
  );
}
