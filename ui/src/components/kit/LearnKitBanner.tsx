import { Badge, Box, Button, Flex, HStack, Text } from "@chakra-ui/react";
import type { LearnKitWizard } from "./useLearnKitWizard";

interface LearnKitBannerProps {
  wizard: LearnKitWizard;
}

export function LearnKitBanner({ wizard }: LearnKitBannerProps) {
  if (!wizard.active) return null;

  const { currentStep, finished, currentIndex, steps } = wizard;
  const total = steps.length;

  return (
    <Flex
      position="absolute"
      bottom={3}
      left="50%"
      transform="translateX(-50%)"
      zIndex={50}
      direction="column"
      gap={2}
      px={5}
      py={3}
      minW="380px"
      maxW="92%"
      bg="rgba(20,20,20,0.94)"
      border="1px solid"
      borderColor={finished ? "#34c759" : "kit.accent"}
      borderRadius="lg"
      boxShadow="0 8px 30px rgba(0,0,0,0.55)"
    >
      {finished ? (
        <>
          <Flex align="center" justify="space-between" gap={4}>
            <HStack spacing={2}>
              <Box boxSize="10px" borderRadius="full" bg="#34c759" />
              <Text fontWeight="semibold">Kit learned</Text>
            </HStack>
            <Button size="sm" colorScheme="green" onClick={wizard.stop}>
              Done
            </Button>
          </Flex>
          <Text fontSize="sm" color="kit.textMuted">
            All {total} mapping{total === 1 ? "" : "s"} captured. You're ready to play.
          </Text>
        </>
      ) : (
        currentStep && (
          <>
            <Flex align="center" justify="space-between" gap={4}>
              <HStack spacing={2}>
                <Box
                  boxSize="10px"
                  borderRadius="full"
                  bg="kit.accent"
                  sx={{
                    animation: "kf-learn-pulse 1.1s ease-in-out infinite",
                    "@keyframes kf-learn-pulse": {
                      "0%, 100%": { opacity: 0.35 },
                      "50%": { opacity: 1 },
                    },
                  }}
                />
                <Text fontWeight="semibold">Listening…</Text>
              </HStack>
              <Text fontSize="xs" color="kit.textMuted">
                Step {currentIndex + 1} of {total}
              </Text>
            </Flex>

            <Flex align="center" gap={2} wrap="wrap">
              <Text fontSize="lg" fontWeight="bold">
                {currentStep.pieceName}
              </Text>
              {currentStep.multiArticulation && (
                <Badge colorScheme="blue" fontSize="0.8em">
                  {currentStep.articulationName}
                </Badge>
              )}
            </Flex>

            <Text fontSize="sm" color="kit.textMuted">
              {currentStep.multiArticulation
                ? `Play the note for the "${currentStep.articulationName}" articulation on your controller.`
                : "Hit the corresponding pad or play the note on your controller."}
            </Text>

            <HStack spacing={2} justify="flex-end" pt={1}>
              <Button size="sm" variant="ghost" onClick={wizard.skip}>
                Skip
              </Button>
              <Button size="sm" variant="outline" borderColor="kit.border" onClick={wizard.stop}>
                Stop
              </Button>
            </HStack>
          </>
        )
      )}
    </Flex>
  );
}
