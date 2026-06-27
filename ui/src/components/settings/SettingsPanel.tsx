import {
  Box,
  Button,
  FormControl,
  FormLabel,
  Heading,
  Input,
  Stack,
  Text,
} from "@chakra-ui/react";

/** Settings UI — persisted via C++ KitForgeSettings when wired. */
export function SettingsPanel() {
  return (
    <Box p={3} h="100%" overflowY="auto">
      <Heading size="sm" mb={3}>
        Settings
      </Heading>
      <Text fontSize="xs" color="kit.textMuted" mb={4}>
        OpenAI API key and catalog URL are stored by the native KitForge settings window in
        standalone mode. Web settings panel is a stub for plugin-embedded configuration.
      </Text>
      <Stack spacing={4}>
        <FormControl>
          <FormLabel fontSize="sm">OpenAI API key</FormLabel>
          <Input type="password" placeholder="sk-..." bg="kit.bg" borderColor="kit.border" />
        </FormControl>
        <FormControl>
          <FormLabel fontSize="sm">Model</FormLabel>
          <Input defaultValue="gpt-4o-mini" bg="kit.bg" borderColor="kit.border" />
        </FormControl>
        <Button size="sm" alignSelf="flex-start" variant="outline" borderColor="kit.border">
          Save (stub)
        </Button>
      </Stack>
    </Box>
  );
}
