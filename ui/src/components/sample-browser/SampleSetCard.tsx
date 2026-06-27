import { Badge, Box, Button, Flex, HStack, Text, Wrap, WrapItem } from "@chakra-ui/react";
import type { SampleSetSummary } from "../../types/kit";
import { SampleSetPreviewButton } from "./SampleSetPreviewButton";

interface SampleSetCardProps {
  set: SampleSetSummary;
  onUse: (set: SampleSetSummary) => void;
}

export function SampleSetCard({ set, onUse }: SampleSetCardProps) {
  return (
    <Box
      bg="kit.bg"
      border="1px solid"
      borderColor={set.allSamplesPresent ? "kit.border" : "orange.500"}
      borderRadius="md"
      p={3}
      opacity={set.allSamplesPresent ? 1 : 0.7}
    >
      <Flex justify="space-between" align="flex-start" gap={2}>
        <Box minW={0}>
          <Text fontWeight="semibold" fontSize="sm" noOfLines={1}>
            {set.displayName}
          </Text>
          <Text fontSize="xs" color="kit.textMuted" noOfLines={1}>
            {set.sourceKitName}
          </Text>
        </Box>
        <HStack spacing={2} flexShrink={0}>
          <SampleSetPreviewButton sampleSetId={set.id} disabled={!set.allSamplesPresent} />
          <Button size="sm" colorScheme="blue" onClick={() => onUse(set)}>
            Use
          </Button>
        </HStack>
      </Flex>

      <HStack spacing={3} mt={2} fontSize="xs" color="kit.textMuted">
        <Text>{set.instrumentType}</Text>
        <Text>·</Text>
        <Text>{set.articulation}</Text>
      </HStack>

      <HStack spacing={2} mt={2}>
        <Badge variant="subtle">{set.velocityLayerCount} vel</Badge>
        <Badge variant="subtle">{set.roundRobinCount} RR</Badge>
        <Badge variant="subtle">{set.sampleCount} smp</Badge>
        {!set.allSamplesPresent && <Badge colorScheme="orange">missing files</Badge>}
      </HStack>

      {set.tags.length > 0 && (
        <Wrap mt={2} spacing={1}>
          {set.tags.slice(0, 6).map((tag) => (
            <WrapItem key={tag}>
              <Badge colorScheme="purple" variant="outline" fontSize="0.65rem">
                {tag}
              </Badge>
            </WrapItem>
          ))}
        </Wrap>
      )}
    </Box>
  );
}
