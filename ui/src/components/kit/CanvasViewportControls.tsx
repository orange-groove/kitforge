import { Box, Flex, Text } from "@chakra-ui/react";

interface CanvasViewportControlsProps {
  zoomPercent: number;
  onZoomIn: () => void;
  onZoomOut: () => void;
  onFit: () => void;
}

const iconBtn = {
  as: "button" as const,
  type: "button" as const,
  display: "flex",
  alignItems: "center",
  justifyContent: "center",
  w: "28px",
  h: "28px",
  flexShrink: 0,
  fontSize: "16px",
  fontWeight: 500,
  lineHeight: 1,
  color: "whiteAlpha.900",
  bg: "transparent",
  border: "none",
  borderRadius: "md",
  cursor: "pointer",
  transition: "background 0.12s ease",
  _hover: { bg: "whiteAlpha.150" },
  _active: { bg: "whiteAlpha.250" },
};

export function CanvasViewportControls({
  zoomPercent,
  onZoomIn,
  onZoomOut,
  onFit,
}: CanvasViewportControlsProps) {
  return (
    <Flex
      position="absolute"
      bottom={3}
      left={3}
      zIndex={20}
      align="center"
      h="34px"
      pl={1}
      pr={1.5}
      gap={0.5}
      bg="blackAlpha.700"
      backdropFilter="blur(10px)"
      border="1px solid"
      borderColor="whiteAlpha.200"
      borderRadius="lg"
      boxShadow="0 4px 20px rgba(0, 0, 0, 0.45)"
      onPointerDown={(e) => e.stopPropagation()}
    >
      <Box {...iconBtn} onClick={onZoomOut} aria-label="Zoom out">
        −
      </Box>

      <Text
        fontSize="11px"
        fontWeight={600}
        letterSpacing="0.02em"
        color="whiteAlpha.800"
        minW="44px"
        textAlign="center"
        userSelect="none"
        sx={{ fontVariantNumeric: "tabular-nums" }}
      >
        {zoomPercent}%
      </Text>

      <Box {...iconBtn} onClick={onZoomIn} aria-label="Zoom in">
        +
      </Box>

      <Box w="1px" h="18px" bg="whiteAlpha.200" mx={0.5} flexShrink={0} aria-hidden />

      <Box
        as="button"
        type="button"
        h="28px"
        px={2.5}
        fontSize="11px"
        fontWeight={600}
        letterSpacing="0.04em"
        textTransform="uppercase"
        color="whiteAlpha.900"
        bg="transparent"
        border="none"
        borderRadius="md"
        cursor="pointer"
        transition="background 0.12s ease, color 0.12s ease"
        _hover={{ bg: "whiteAlpha.150", color: "kit.accent" }}
        _active={{ bg: "whiteAlpha.250" }}
        onClick={onFit}
        aria-label="Fit drum kit to view"
      >
        Fit
      </Box>
    </Flex>
  );
}
