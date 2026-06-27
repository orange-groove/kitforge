import {
  Box,
  Button,
  Flex,
  Heading,
  Stack,
  Tag,
  Text,
} from "@chakra-ui/react";
import type { InstalledKit } from "../../types/kit";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

interface LibraryBrowserPanelProps {
  installed: InstalledKit[];
  onMapLibrary: (kitId: string) => void;
}

export function LibraryBrowserPanel({ installed, onMapLibrary }: LibraryBrowserPanelProps) {
  return (
    <Box p={3}>
      <Heading size="sm" mb={3}>
        Installed Kits
      </Heading>

      <Flex gap={2} mb={4} flexWrap="wrap">
        <Button size="sm" colorScheme="blue" onClick={() => kitforgeBridge.importSfz()}>
          Import SFZ
        </Button>
        <Button size="sm" variant="outline" borderColor="kit.border" onClick={() => kitforgeBridge.importLooseFolder()}>
          Import Folder
        </Button>
      </Flex>

      <Stack spacing={3}>
        {installed.map((kit) => (
          <Box
            key={kit.id}
            p={3}
            bg="kit.bg"
            borderRadius="md"
            border="1px solid"
            borderColor="kit.border"
          >
            <Flex justify="space-between" align="start" gap={2}>
              <Box flex="1">
                <Text fontWeight="bold" fontSize="sm">
                  {kit.name}
                </Text>
                <Text fontSize="xs" color="kit.textMuted" mt={1}>
                  {kit.author || "Unknown author"} · v{kit.version || "1.0.0"}
                </Text>
                <Flex gap={2} mt={2} wrap="wrap">
                  <Tag size="sm">{kit.license || "custom"}</Tag>
                  <Tag size="sm">{kit.sampleCount} samples</Tag>
                  {kit.pieceCount > 0 && <Tag size="sm">{kit.pieceCount} pieces</Tag>}
                </Flex>
                {kit.tags.length > 0 && (
                  <Text fontSize="xs" color="kit.textMuted" mt={1}>
                    {kit.tags.join(", ")}
                  </Text>
                )}
              </Box>
              <Flex direction="column" gap={1}>
                <Button
                  size="sm"
                  colorScheme="blue"
                  onClick={() => kitforgeBridge.loadInstalledKit(kit.id)}
                >
                  Load Kit
                </Button>
                <Button
                  size="sm"
                  variant="outline"
                  borderColor="kit.border"
                  onClick={() => onMapLibrary(kit.id)}
                >
                  Map
                </Button>
                <Button
                  size="sm"
                  variant="outline"
                  borderColor="kit.border"
                  onClick={() => kitforgeBridge.revealKit(kit.id)}
                >
                  Reveal
                </Button>
                <Button
                  size="sm"
                  variant="outline"
                  colorScheme="red"
                  borderColor="kit.border"
                  onClick={() => kitforgeBridge.removeKit(kit.id)}
                >
                  Remove
                </Button>
              </Flex>
            </Flex>
          </Box>
        ))}
        {installed.length === 0 && (
          <Text fontSize="sm" color="kit.textMuted">
            No installed kits yet. Import an SFZ or a WAV folder to get started.
          </Text>
        )}
      </Stack>
    </Box>
  );
}
