import { useState } from "react";
import {
  Box,
  Button,
  Heading,
  Text,
  Textarea,
  Stack,
} from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

interface AIBuilderPanelProps {
  loading: boolean;
  lastMessage: string | null;
  onGenerate?: () => void;
}

export function AIBuilderPanel({ loading, lastMessage, onGenerate }: AIBuilderPanelProps) {
  const [prompt, setPrompt] = useState("Build me a tight modern metal kit");

  return (
    <Box p={3} h="100%">
      <Heading size="sm" mb={2}>
        AI Kit Builder
      </Heading>
      <Text fontSize="xs" color="kit.textMuted" mb={3}>
        Describe the kit layout and MIDI mapping.
      </Text>
      <Stack spacing={3}>
        <Textarea
          value={prompt}
          onChange={(e) => setPrompt(e.target.value)}
          rows={6}
          bg="kit.bg"
          borderColor="kit.border"
          fontSize="sm"
        />
        <Button
          colorScheme="blue"
          isLoading={loading}
          onClick={() => {
            onGenerate?.();
            kitforgeBridge.aiBuildKit(prompt);
          }}
        >
          Generate Kit
        </Button>
        {lastMessage && (
          <Text fontSize="sm" color={lastMessage.startsWith("Error") ? "red.300" : "kit.textMuted"}>
            {lastMessage}
          </Text>
        )}
      </Stack>
    </Box>
  );
}
