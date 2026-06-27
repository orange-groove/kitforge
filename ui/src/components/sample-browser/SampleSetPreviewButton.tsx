import { IconButton } from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

interface SampleSetPreviewButtonProps {
  sampleSetId: string;
  disabled?: boolean;
}

/** Triggers a one-shot preview of a sample set through the native engine. */
export function SampleSetPreviewButton({
  sampleSetId,
  disabled,
}: SampleSetPreviewButtonProps) {
  return (
    <IconButton
      aria-label="Preview sample set"
      size="sm"
      variant="outline"
      borderColor="kit.border"
      isDisabled={disabled}
      onClick={(e) => {
        e.stopPropagation();
        kitforgeBridge.previewSampleSet(sampleSetId);
      }}
      icon={<span style={{ fontSize: "0.9em" }}>▶</span>}
    />
  );
}
