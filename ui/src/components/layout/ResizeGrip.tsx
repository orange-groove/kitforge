import { useRef } from "react";
import { Box } from "@chakra-ui/react";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";

/**
 * A draggable resize handle pinned to the bottom-right of the plugin window.
 *
 * Some hosts (notably FL Studio attached mode on macOS) put the native WebView on
 * top of the editor, hiding the host/JUCE resize corner. Because this grip is web
 * content it can never be covered, and on drag it asks the C++ editor to resize.
 */
export function ResizeGrip() {
  const dragState = useRef<{
    startX: number;
    startY: number;
    startW: number;
    startH: number;
  } | null>(null);
  const frame = useRef<number | null>(null);
  const pending = useRef<{ w: number; h: number } | null>(null);

  const flush = () => {
    frame.current = null;
    if (pending.current) {
      kitforgeBridge.resizeEditor(pending.current.w, pending.current.h);
      pending.current = null;
    }
  };

  const onPointerDown = (e: React.PointerEvent) => {
    e.preventDefault();
    e.stopPropagation();
    (e.target as Element).setPointerCapture(e.pointerId);
    dragState.current = {
      startX: e.clientX,
      startY: e.clientY,
      startW: window.innerWidth,
      startH: window.innerHeight,
    };
  };

  const onPointerMove = (e: React.PointerEvent) => {
    const s = dragState.current;
    if (!s) return;
    pending.current = {
      w: s.startW + (e.clientX - s.startX),
      h: s.startH + (e.clientY - s.startY),
    };
    if (frame.current == null) {
      frame.current = window.requestAnimationFrame(flush);
    }
  };

  const endDrag = (e: React.PointerEvent) => {
    if (!dragState.current) return;
    dragState.current = null;
    if (frame.current != null) {
      window.cancelAnimationFrame(frame.current);
      frame.current = null;
    }
    flush();
    try {
      (e.target as Element).releasePointerCapture(e.pointerId);
    } catch {
      /* pointer already released */
    }
  };

  return (
    <Box
      position="absolute"
      right="0"
      bottom="0"
      w="16px"
      h="16px"
      cursor="nwse-resize"
      zIndex={50}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={endDrag}
      onPointerCancel={endDrag}
      sx={{
        // Diagonal grip lines, drawn in the very corner.
        backgroundImage:
          "linear-gradient(135deg, transparent 0 45%, var(--chakra-colors-whiteAlpha-400) 45% 55%, transparent 55% 70%, var(--chakra-colors-whiteAlpha-400) 70% 80%, transparent 80%)",
      }}
    />
  );
}
