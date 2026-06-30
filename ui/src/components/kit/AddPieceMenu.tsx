import {
  Menu,
  MenuButton,
  MenuDivider,
  MenuGroup,
  MenuItem,
  MenuList,
  Text,
} from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";
import type { DrumPieceType } from "../../types/kit";
import {
  CYMBAL_ADD_OPTIONS,
  DRUM_ADD_OPTIONS,
  PIECE_DIAMETERS_IN,
  type PieceDiameterIn,
} from "./addPieceCatalog";

function PieceSizeSubmenu({ label, pieceType }: { label: string; pieceType: DrumPieceType }) {
  const add = (diameterInches: PieceDiameterIn) => {
    kitforgeBridge.addPiece(pieceType, diameterInches);
  };

  return (
    <Menu placement="right-start" isLazy gutter={2}>
      <MenuButton
        as={MenuItem}
        closeOnSelect={false}
        _hover={{ bg: "kit.border" }}
        _focus={{ bg: "kit.border" }}
      >
        <Text as="span" flex="1">
          {label}
        </Text>
        <Text as="span" color="kit.textMuted" fontSize="xs" ml={2}>
          ›
        </Text>
      </MenuButton>
      <MenuList minW="72px" py={1}>
        {PIECE_DIAMETERS_IN.map((d) => (
          <MenuItem key={d} fontSize="sm" onClick={() => add(d)}>
            {d}&Prime;
          </MenuItem>
        ))}
      </MenuList>
    </Menu>
  );
}

/** Nested Kit → Add Piece menu with type and diameter (inches). */
export function AddPieceMenuItems() {
  return (
    <>
      <MenuGroup title="Drums" fontSize="xs" color="kit.textMuted">
        {DRUM_ADD_OPTIONS.map((opt) => (
          <PieceSizeSubmenu key={opt.type} label={opt.label} pieceType={opt.type} />
        ))}
      </MenuGroup>
      <MenuDivider borderColor="kit.border" />
      <MenuGroup title="Cymbals" fontSize="xs" color="kit.textMuted">
        {CYMBAL_ADD_OPTIONS.map((opt) => (
          <PieceSizeSubmenu key={opt.type} label={opt.label} pieceType={opt.type} />
        ))}
      </MenuGroup>
    </>
  );
}
