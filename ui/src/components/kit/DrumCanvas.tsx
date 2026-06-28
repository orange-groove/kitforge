import { useCallback, useEffect, useRef, useState } from "react";
import { Box, Button, Flex, HStack, Text } from "@chakra-ui/react";
import type { DrumPiece, KitModel, SwapTarget } from "../../types/kit";
import { kitforgeBridge } from "../../bridge/kitforgeBridge";
import kickSvgRaw from "../../assets/kick.svg?raw";

const kickSvgUrl = `data:image/svg+xml;charset=utf-8,${encodeURIComponent(kickSvgRaw)}`;
import {
  argbToCss,
  computeFitViewportForKit,
  isCymbalPiece,
  isKickPiece,
  isRidePiece,
  layoutReferenceSize,
  pieceCircleGeometry,
  rideBellArticulation,
  rideEdgeArticulation,
  rideArticulationFromPoint,
  RIDE_BELL_R,
  RIDE_BELL_HIT_R,
  RIDE_EDGE_MIDI_Y,
  type RideHitZone,
  screenToNormalizedCenter,
  type Viewport,
  zoomAtPoint,
} from "./layoutUtils";

interface DrumCanvasProps {
  kit: KitModel;
  selectedPieceId: string | null;
  selectedArticulationId: string | null;
  editLayout: boolean;
  onSelectPiece: (pieceId: string | null, articulationId?: string | null) => void;
  onSwapSamples: (target: SwapTarget) => void;
  /** Piece currently listening in the Learn My Kit wizard (blue highlight). */
  learnActivePieceId?: string | null;
  /** Pieces already learned in the wizard (green highlight). */
  learnDonePieceIds?: Set<string>;
}

const CLICK_SLOP_PX = 6;

const DRUM_RIM_COLOR = "#231f20";
const DRUM_SELECTED_RIM_COLOR = "#5b8def";

/**
 * Inline render of `assets/drum.svg` so the head and rim can be recolored at
 * runtime. The lug hardware is fixed; `headFill` reflects the hit/rest state and
 * `rimFill` turns blue when the piece is selected.
 */
function DrumGraphic({
  x,
  y,
  size,
  headFill,
  rimFill,
}: {
  x: number;
  y: number;
  size: number;
  headFill: string;
  rimFill: string;
}) {
  return (
    <svg
      x={x}
      y={y}
      width={size}
      height={size}
      viewBox="0 0 160.79 160.79"
      preserveAspectRatio="xMidYMid meet"
      style={{ pointerEvents: "none", overflow: "visible" }}
    >
      <g fill="#fff" stroke="#231f20" strokeMiterlimit={10}>
        <circle cx="80.24" cy="5.28" r="3.25" />
        <circle cx="80.55" cy="155.51" r="3.25" />
        <circle cx="5.28" cy="80.55" r="3.25" />
        <circle cx="155.51" cy="80.24" r="3.25" />
        <path d="M29.47,29.69c-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6c1.27-1.26,3.33-1.26,4.6,0,1.27,1.27,1.27,3.33,0,4.6Z" />
        <path d="M135.92,135.7c-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6,3.33-1.27,4.6,0c1.27,1.27,1.27,3.33,0,4.6Z" />
        <path d="M29.69,135.92c-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6c1.27-1.27,3.33-1.27,4.6,0,1.27,1.27,1.27,3.33,0,4.6Z" />
        <path d="M135.69,29.47c-1.26,1.27-3.32,1.27-4.59,0-1.27-1.27-1.27-3.33,0-4.6s3.33-1.27,4.59,0c1.27,1.27,1.27,3.33,0,4.6Z" />
        <path d="M150.42,68.7c-.33,0-.65.02-.97.06-1.78-10.67-5.97-20.52-12-28.97.25-.19.5-.41.73-.64,3.85-3.86,3.27-10.68-1.29-15.25-4.57-4.57-11.39-5.14-15.25-1.29-.23.23-.45.48-.64.73-8.45-6.03-18.3-10.22-28.97-12,.04-.32.06-.64.06-.97,0-5.45-5.24-9.87-11.7-9.87s-11.69,4.42-11.69,9.87c0,.33.02.65.06.97-10.67,1.77-20.52,5.97-28.98,12-.19-.25-.41-.5-.64-.73-3.85-3.85-10.67-3.28-15.24,1.29-4.57,4.57-5.15,11.39-1.29,15.25.23.23.47.44.72.64-6.03,8.45-10.22,18.3-12,28.97-.32-.04-.64-.06-.97-.06-5.44,0-9.86,5.24-9.86,11.7s4.42,11.69,9.86,11.69c.33,0,.65-.02.97-.06,1.78,10.67,5.97,20.52,12,28.98-.25.2-.49.41-.72.64-3.86,3.85-3.28,10.67,1.29,15.24s11.39,5.15,15.24,1.29c.23-.23.44-.47.64-.72,8.46,6.03,18.31,10.23,28.98,12-.04.32-.06.64-.06.97,0,5.44,5.23,9.86,11.69,9.86s11.7-4.42,11.7-9.86c0-.33-.02-.65-.06-.97,10.67-1.78,20.52-5.97,28.97-12,.2.25.41.49.64.72,3.86,3.86,10.68,3.28,15.25-1.29,4.56-4.57,5.14-11.39,1.29-15.24-.23-.23-.48-.45-.73-.64,6.03-8.46,10.22-18.31,12-28.98.32.04.64.06.97.06,5.45,0,9.87-5.24,9.87-11.69s-4.42-11.7-9.87-11.7ZM5.28,83.8c-1.8,0-3.25-1.45-3.25-3.25s1.45-3.25,3.25-3.25,3.25,1.46,3.25,3.25-1.46,3.25-3.25,3.25ZM24.87,25.09c1.27-1.26,3.33-1.26,4.6,0,1.27,1.27,1.27,3.33,0,4.6-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6ZM29.69,135.92c-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6c1.27-1.27,3.33-1.27,4.6,0,1.27,1.27,1.27,3.33,0,4.6ZM80.24,2.03c1.79,0,3.25,1.46,3.25,3.25s-1.46,3.25-3.25,3.25-3.25-1.45-3.25-3.25,1.45-3.25,3.25-3.25ZM80.55,158.76c-1.79,0-3.25-1.45-3.25-3.25s1.46-3.25,3.25-3.25,3.25,1.46,3.25,3.25-1.46,3.25-3.25,3.25ZM131.1,24.87c1.27-1.27,3.33-1.27,4.59,0,1.27,1.27,1.27,3.33,0,4.6-1.26,1.27-3.32,1.27-4.59,0-1.27-1.27-1.27-3.33,0-4.6ZM135.92,135.7c-1.27,1.27-3.33,1.27-4.6,0s-1.27-3.33,0-4.6,3.33-1.27,4.6,0c1.27,1.27,1.27,3.33,0,4.6ZM155.51,83.49c-1.8,0-3.25-1.46-3.25-3.25s1.45-3.25,3.25-3.25,3.25,1.45,3.25,3.25-1.46,3.25-3.25,3.25Z" />
      </g>
      <circle cx="80.24" cy="80.24" r="67.5" fill={rimFill} stroke={rimFill} strokeMiterlimit={10} />
      <circle cx="80.55" cy="80.55" r="64.35" fill={headFill} />
    </svg>
  );
}

type PendingInteraction = {
  pointerId: number;
  startX: number;
  startY: number;
  panX: number;
  panY: number;
  pieceId: string | null;
  onClick?: () => void;
};

export function DrumCanvas({
  kit,
  selectedPieceId,
  selectedArticulationId,
  editLayout,
  onSelectPiece,
  onSwapSamples,
  learnActivePieceId = null,
  learnDonePieceIds,
}: DrumCanvasProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [size, setSize] = useState({ w: 800, h: 600 });
  const [viewport, setViewport] = useState<Viewport>({ zoom: 1, panX: 0, panY: 0 });
  const viewportRef = useRef(viewport);
  viewportRef.current = viewport;

  const panDrag = useRef<{ startX: number; startY: number; panX: number; panY: number } | null>(null);
  const pendingInteraction = useRef<PendingInteraction | null>(null);
  const pieceDrag = useRef<{
    pieceId: string;
    pointerId: number;
    grabDX: number;
    grabDY: number;
  } | null>(null);
  const layoutDragPos = useRef<Record<string, { x: number; y: number }>>({});
  const [, bumpLayoutDrag] = useState(0);

  const pieceForDisplay = useCallback(
    (piece: DrumPiece): DrumPiece => {
      const drag = layoutDragPos.current[piece.id];
      if (!drag) return piece;
      return {
        ...piece,
        x: drag.x - piece.width * 0.5,
        y: drag.y - piece.height * 0.5,
      };
    },
    [],
  );

  const [hitPieceIds, setHitPieceIds] = useState<Set<string>>(() => new Set());
  const hitTimers = useRef<Record<string, ReturnType<typeof setTimeout>>>({});

  const flashHit = useCallback((pieceId: string) => {
    setHitPieceIds((prev) => {
      const next = new Set(prev);
      next.add(pieceId);
      return next;
    });
    const existing = hitTimers.current[pieceId];
    if (existing) clearTimeout(existing);
    hitTimers.current[pieceId] = setTimeout(() => {
      setHitPieceIds((prev) => {
        if (!prev.has(pieceId)) return prev;
        const next = new Set(prev);
        next.delete(pieceId);
        return next;
      });
      delete hitTimers.current[pieceId];
    }, 200);
  }, []);

  useEffect(() => {
    const timers = hitTimers.current;
    return () => {
      Object.values(timers).forEach((t) => clearTimeout(t));
    };
  }, []);

  const [menuPiece, setMenuPiece] = useState<DrumPiece | null>(null);
  const [menuArticulationId, setMenuArticulationId] = useState<string | null>(null);
  const [openSubmenu, setOpenSubmenu] = useState<string | null>(null);
  const [menuPos, setMenuPos] = useState({ x: 0, y: 0 });

  const { refW, refH } = layoutReferenceSize(kit);
  const piecesRef = useRef(kit.pieces);
  piecesRef.current = kit.pieces;

  const applyFitToView = useCallback(() => {
    const el = containerRef.current;
    if (!el) return;
    const w = el.clientWidth;
    const h = el.clientHeight;
    if (w <= 0 || h <= 0) return;
    setViewport(computeFitViewportForKit(piecesRef.current, w, h, refW, refH));
  }, [refW, refH]);

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const ro = new ResizeObserver((entries) => {
      const cr = entries[0]?.contentRect;
      if (cr) {
        setSize({ w: cr.width, h: cr.height });
      }
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  useEffect(() => {
    applyFitToView();
  }, [kit.kitName, refW, refH, applyFitToView]);

  useEffect(() => {
    if (size.w <= 0 || size.h <= 0) return;
    applyFitToView();
  }, [size.w, size.h, applyFitToView]);

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const rect = el.getBoundingClientRect();
      const factor = e.deltaY > 0 ? 0.9 : 1.1;
      setViewport((v) => zoomAtPoint(v, e.clientX, e.clientY, rect, factor));
    };

    el.addEventListener("wheel", onWheel, { passive: false });
    return () => el.removeEventListener("wheel", onWheel);
  }, []);

  const fitView = () => {
    applyFitToView();
  };

  const nudgeZoom = (factor: number) => {
    const el = containerRef.current;
    if (!el) return;
    const rect = el.getBoundingClientRect();
    const cx = rect.left + rect.width * 0.5;
    const cy = rect.top + rect.height * 0.5;
    setViewport((v) => zoomAtPoint(v, cx, cy, rect, factor));
  };

  const startPan = (clientX: number, clientY: number) => {
    panDrag.current = {
      startX: clientX,
      startY: clientY,
      panX: viewportRef.current.panX,
      panY: viewportRef.current.panY,
    };
  };

  const findPieceIdFromTarget = (target: EventTarget | null): string | null => {
    let node = target as Element | null;
    while (node) {
      const pieceId = node.getAttribute?.("data-piece-id");
      if (pieceId) return pieceId;
      node = node.parentElement;
    }
    return null;
  };

  const findRideZoneFromTarget = (target: EventTarget | null): RideHitZone | null => {
    let node = target as Element | null;
    while (node) {
      const zone = node.getAttribute?.("data-ride-zone");
      if (zone === "bell" || zone === "edge") return zone;
      node = node.parentElement;
    }
    return null;
  };

  const resolveRideArticulationId = (
    piece: DrumPiece,
    target: EventTarget | null,
    clientX: number,
    clientY: number,
    refWidth: number,
    refHeight: number,
  ): string | null => {
    const edgeArt = rideEdgeArticulation(piece);
    const bellArt = rideBellArticulation(piece);
    if (!edgeArt) return null;

    const zone = findRideZoneFromTarget(target);
    if (zone === "bell" && bellArt != null) return bellArt.id;
    if (zone === "edge") return edgeArt.id;

    const rect = containerRef.current?.getBoundingClientRect();
    if (!rect) return edgeArt.id;

    return (
      rideArticulationFromPoint(
        piece,
        clientX,
        clientY,
        rect,
        viewportRef.current,
        refWidth,
        refHeight,
      ) ?? edgeArt.id
    );
  };

  const buildClickHandler = (
    target: EventTarget | null,
    pieces: DrumPiece[],
    refWidth: number,
    refHeight: number,
    downX: number,
    downY: number,
  ): (() => void) | undefined => {
    const pieceId = findPieceIdFromTarget(target);
    if (!pieceId) return undefined;

    const piece = pieces.find((p) => p.id === pieceId);
    if (!piece) return undefined;

    return () => {
      if (editLayout) {
        onSelectPiece(piece.id, null);
        return;
      }

      if (isRidePiece(piece)) {
        const articulationId = resolveRideArticulationId(
          piece,
          target,
          downX,
          downY,
          refWidth,
          refHeight,
        );
        if (!articulationId) return;

        onSelectPiece(piece.id, articulationId);
        flashHit(piece.id);
        kitforgeBridge.triggerPiece(piece.id, articulationId);
        return;
      }

      onSelectPiece(piece.id, null);
      flashHit(piece.id);
      kitforgeBridge.triggerPiece(piece.id);
    };
  };

  const handlePointerDown = (e: React.PointerEvent) => {
    if (e.button === 1 || (e.button === 0 && e.altKey)) {
      pendingInteraction.current = null;
      startPan(e.clientX, e.clientY);
      containerRef.current?.setPointerCapture(e.pointerId);
      return;
    }

    if (e.button !== 0 || editLayout) return;

    const pieceId = findPieceIdFromTarget(e.target);
    const piece = pieceId ? kit.pieces.find((p) => p.id === pieceId) : undefined;
    let onClick = buildClickHandler(e.target, kit.pieces, refW, refH, e.clientX, e.clientY);

    if (piece && isRidePiece(piece)) {
      const articulationId = resolveRideArticulationId(
        piece,
        e.target,
        e.clientX,
        e.clientY,
        refW,
        refH,
      );
      if (articulationId) {
        onSelectPiece(piece.id, articulationId);
        flashHit(piece.id);
        kitforgeBridge.triggerPiece(piece.id, articulationId);
        const ridePieceId = piece.id;
        onClick = () => {
          onSelectPiece(ridePieceId, articulationId);
        };
      }
    }

    pendingInteraction.current = {
      pointerId: e.pointerId,
      startX: e.clientX,
      startY: e.clientY,
      panX: viewportRef.current.panX,
      panY: viewportRef.current.panY,
      pieceId: pieceId ?? null,
      onClick,
    };
    containerRef.current?.setPointerCapture(e.pointerId);
  };

  const handlePointerMove = (e: React.PointerEvent) => {
    const pending = pendingInteraction.current;
    if (pending && pending.pointerId === e.pointerId && !panDrag.current) {
      const dx = e.clientX - pending.startX;
      const dy = e.clientY - pending.startY;
      if (Math.hypot(dx, dy) >= CLICK_SLOP_PX) {
        panDrag.current = {
          startX: pending.startX,
          startY: pending.startY,
          panX: pending.panX,
          panY: pending.panY,
        };
        pendingInteraction.current = null;
      }
    }

    if (panDrag.current) {
      const dx = e.clientX - panDrag.current.startX;
      const dy = e.clientY - panDrag.current.startY;
      setViewport((v) => ({
        ...v,
        panX: panDrag.current!.panX + dx,
        panY: panDrag.current!.panY + dy,
      }));
      return;
    }

    if (pieceDrag.current && editLayout) {
      const rect = containerRef.current?.getBoundingClientRect();
      if (!rect) return;
      const pointer = screenToNormalizedCenter(
        e.clientX,
        e.clientY,
        rect,
        viewportRef.current,
        refW,
        refH,
      );
      const center = {
        x: pointer.x - pieceDrag.current.grabDX,
        y: pointer.y - pieceDrag.current.grabDY,
      };
      layoutDragPos.current = {
        ...layoutDragPos.current,
        [pieceDrag.current.pieceId]: center,
      };
      bumpLayoutDrag((n) => n + 1);
    }
  };

  const handlePointerUp = (e: React.PointerEvent) => {
    const pending = pendingInteraction.current;
    if (pending && pending.pointerId === e.pointerId && !panDrag.current) {
      const dx = e.clientX - pending.startX;
      const dy = e.clientY - pending.startY;
      if (Math.hypot(dx, dy) < CLICK_SLOP_PX) {
        if (pending.onClick) {
          pending.onClick();
        } else if (pending.pieceId == null) {
          if (menuPiece) setMenuPiece(null);
          onSelectPiece(null, null);
        }
      }
    }

    panDrag.current = null;
    pendingInteraction.current = null;

    if (pieceDrag.current && editLayout) {
      const { pieceId } = pieceDrag.current;
      const drag = layoutDragPos.current[pieceId];
      if (drag) {
        kitforgeBridge.movePiece(pieceId, drag.x, drag.y, true);
        delete layoutDragPos.current[pieceId];
        bumpLayoutDrag((n) => n + 1);
      }
    }

    pieceDrag.current = null;
    const el = containerRef.current;
    if (el?.hasPointerCapture(e.pointerId)) {
      el.releasePointerCapture(e.pointerId);
    }
  };

  const cursor =
    panDrag.current != null || pendingInteraction.current != null
      ? "grabbing"
      : editLayout
        ? "default"
        : "grab";

  return (
    <Box
      ref={containerRef}
      flex="1"
      position="relative"
      borderRadius="md"
      border="1px solid"
      borderColor="kit.border"
      overflow="hidden"
      bg="#141414"
      sx={{ touchAction: "none" }}
      cursor={cursor}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onPointerCancel={handlePointerUp}
    >
      <svg
        width={size.w}
        height={size.h}
        viewBox={`0 0 ${size.w} ${size.h}`}
        preserveAspectRatio="xMidYMid meet"
        style={{ display: "block", shapeRendering: "geometricPrecision", flexShrink: 0 }}
      >
        <defs>
          <radialGradient
            id="kitforge-canvas-bg"
            gradientUnits="userSpaceOnUse"
            cx={size.w * 0.5}
            cy={size.h * 0.38}
            r={Math.max(size.w, size.h) * 0.95}
          >
            <stop offset="0%" stopColor="#282828" />
            <stop offset="55%" stopColor="#1c1c1c" />
            <stop offset="100%" stopColor="#141414" />
          </radialGradient>
          <radialGradient id="kitforge-drumhead-hit" cx="50%" cy="42%" r="62%">
            <stop offset="0%" stopColor="#ffffff" />
            <stop offset="55%" stopColor="#ededed" />
            <stop offset="100%" stopColor="#c8c8c8" />
          </radialGradient>
        </defs>

        <rect x={0} y={0} width={size.w} height={size.h} fill="url(#kitforge-canvas-bg)" />

        <g transform={`translate(${viewport.panX}, ${viewport.panY}) scale(${viewport.zoom})`}>
          {kit.pieces.map((piece) => {
            const displayPiece = pieceForDisplay(piece);
            const { cx, cy, r } = pieceCircleGeometry(displayPiece, refW, refH);
            const selected = selectedPieceId === piece.id;
            const hit = hitPieceIds.has(piece.id);
            const learnListening = learnActivePieceId === piece.id;
            const learnDone = learnDonePieceIds?.has(piece.id) ?? false;
            const cymbal = isCymbalPiece(piece);
            const kick = isKickPiece(piece);
            const isRide = isRidePiece(piece);
            const edgeArt = isRide ? rideEdgeArticulation(piece) : undefined;
            const bellArt = isRide ? rideBellArticulation(piece) : undefined;
            const bellSelected =
              selected && bellArt != null && selectedArticulationId === bellArt.id;
            const edgeSelected =
              selected && edgeArt != null && selectedArticulationId === edgeArt.id;
            const fill = argbToCss(piece.color);
            const labelColor = cymbal ? "#f0f0f0" : "#111111";
            const strokeW = (w: number) => w / viewport.zoom;
            const selectionPad = strokeW(2);
            const edgeMidi = edgeArt?.midiNote ?? piece.primaryMidiNote;

            const openContextMenu = (
              e: React.MouseEvent,
              articulationId?: string,
            ) => {
              e.preventDefault();
              e.stopPropagation();
              onSelectPiece(piece.id, articulationId ?? null);
              setMenuPiece(piece);
              setMenuArticulationId(articulationId ?? edgeArt?.id ?? piece.articulations[0]?.id ?? null);
              setOpenSubmenu(null);
              setMenuPos({ x: e.clientX, y: e.clientY });
            };

            const handleRideContextMenu = (e: React.MouseEvent, clientX: number, clientY: number) => {
              if (!isRide || !edgeArt) return;

              e.preventDefault();
              e.stopPropagation();

              const rect = containerRef.current?.getBoundingClientRect();
              if (!rect) return;

              const articulationId =
                rideArticulationFromPoint(
                  piece,
                  clientX,
                  clientY,
                  rect,
                  viewportRef.current,
                  refW,
                  refH,
                ) ?? edgeArt.id;

              onSelectPiece(piece.id, articulationId);
              setMenuPiece(piece);
              setMenuArticulationId(articulationId);
              setOpenSubmenu(null);
              setMenuPos({ x: e.clientX, y: e.clientY });
            };

            return (
              <g
                key={piece.id}
                data-piece-id={piece.id}
                style={{ cursor: editLayout ? "grab" : "inherit" }}
                onContextMenu={(e) => {
                  if (isRide) {
                    handleRideContextMenu(e, e.clientX, e.clientY);
                    return;
                  }
                  openContextMenu(e);
                }}
              >
                {edgeSelected && (
                  <circle
                    cx={cx}
                    cy={cy}
                    r={r + selectionPad}
                    fill="none"
                    stroke="#5b8def"
                    strokeWidth={strokeW(3)}
                    style={{ pointerEvents: "none" }}
                  />
                )}

                {selected && cymbal && !isRide && (
                  <circle
                    cx={cx}
                    cy={cy}
                    r={r + selectionPad}
                    fill="none"
                    stroke="#5b8def"
                    strokeWidth={strokeW(3)}
                    style={{ pointerEvents: "none" }}
                  />
                )}

                {selected && kick && (
                  <rect
                    x={cx - r - selectionPad}
                    y={cy - r - selectionPad}
                    width={2 * (r + selectionPad)}
                    height={2 * (r + selectionPad)}
                    fill="none"
                    stroke="#5b8def"
                    strokeWidth={strokeW(3)}
                    style={{ pointerEvents: "none" }}
                  />
                )}

                {kick ? (
                  <image
                    href={kickSvgUrl}
                    x={cx - r}
                    y={cy - r}
                    width={2 * r}
                    height={2 * r}
                    preserveAspectRatio="xMidYMid meet"
                    style={{ pointerEvents: "none" }}
                  />
                ) : !cymbal ? (
                  <DrumGraphic
                    x={cx - r}
                    y={cy - r}
                    size={2 * r}
                    headFill={hit ? "url(#kitforge-drumhead-hit)" : fill}
                    rimFill={selected ? DRUM_SELECTED_RIM_COLOR : DRUM_RIM_COLOR}
                  />
                ) : (
                  <>
                    <circle
                      cx={cx}
                      cy={cy}
                      r={r}
                      fill={fill}
                      stroke="#000000"
                      strokeWidth={strokeW(2)}
                      style={{ pointerEvents: "none" }}
                    />
                    {isRide && edgeArt && bellArt && (
                      <>
                        {bellSelected && (
                          <circle
                            cx={cx}
                            cy={cy}
                            r={r * RIDE_BELL_R + strokeW(2)}
                            fill="none"
                            stroke="#5b8def"
                            strokeWidth={strokeW(2.5)}
                            style={{ pointerEvents: "none" }}
                          />
                        )}
                        <circle
                          cx={cx}
                          cy={cy}
                          r={r * RIDE_BELL_R}
                          fill="#787878"
                          stroke="#000000"
                          strokeWidth={strokeW(2)}
                          style={{ pointerEvents: "none" }}
                        />
                        <text
                          x={cx}
                          y={cy + 4}
                          textAnchor="middle"
                          fill="#f0f0f0"
                          fontSize={Math.max(10, r * RIDE_BELL_R * 0.55)}
                          fontWeight={700}
                          style={{ pointerEvents: "none", userSelect: "none" }}
                        >
                          {bellArt.midiNote}
                        </text>
                        <text
                          x={cx}
                          y={cy + r * RIDE_EDGE_MIDI_Y}
                          textAnchor="middle"
                          fill="#b0b0b0"
                          fontSize={Math.max(10, r * 0.18)}
                          fontWeight={700}
                          style={{ pointerEvents: "none", userSelect: "none" }}
                        >
                          {edgeMidi}
                        </text>
                      </>
                    )}
                  </>
                )}

                <text
                  x={cx}
                  y={isRide ? cy - r * 0.42 : cy - 4}
                  textAnchor="middle"
                  fill={labelColor}
                  fontSize={Math.max(8, r * 0.15)}
                  fontWeight={600}
                  style={{ pointerEvents: "none", userSelect: "none" }}
                >
                  {piece.name}
                </text>
                {!isRide && (
                  <text
                    x={cx}
                    y={cy + r * 0.28}
                    textAnchor="middle"
                    fill={cymbal ? "#b0b0b0" : "#444444"}
                    fontSize={Math.max(10, r * 0.2)}
                    fontWeight={700}
                    style={{ pointerEvents: "none", userSelect: "none" }}
                  >
                    {edgeMidi}
                  </text>
                )}

                {!editLayout && isRide && edgeArt && bellArt ? (
                  <>
                    <circle
                      cx={cx}
                      cy={cy}
                      r={r}
                      fill="transparent"
                      data-ride-zone="edge"
                    />
                    <circle
                      cx={cx}
                      cy={cy}
                      r={r * RIDE_BELL_HIT_R}
                      fill="transparent"
                      data-ride-zone="bell"
                    />
                  </>
                ) : (
                  <circle
                    cx={cx}
                    cy={cy}
                    r={r}
                    fill="transparent"
                    onPointerDown={(e) => {
                      if (!editLayout || e.button !== 0) return;
                      e.stopPropagation();
                      pendingInteraction.current = null;
                      const rect = containerRef.current?.getBoundingClientRect();
                      let grabDX = 0;
                      let grabDY = 0;
                      if (rect) {
                        const pointer = screenToNormalizedCenter(
                          e.clientX,
                          e.clientY,
                          rect,
                          viewportRef.current,
                          refW,
                          refH,
                        );
                        grabDX = pointer.x - (displayPiece.x + displayPiece.width * 0.5);
                        grabDY = pointer.y - (displayPiece.y + displayPiece.height * 0.5);
                      }
                      pieceDrag.current = {
                        pieceId: piece.id,
                        pointerId: e.pointerId,
                        grabDX,
                        grabDY,
                      };
                      containerRef.current?.setPointerCapture(e.pointerId);
                      onSelectPiece(piece.id, edgeArt?.id ?? null);
                    }}
                  />
                )}

                {(learnListening || learnDone) &&
                  (kick ? (
                    <rect
                      x={cx - r - selectionPad * 2}
                      y={cy - r - selectionPad * 2}
                      width={2 * (r + selectionPad * 2)}
                      height={2 * (r + selectionPad * 2)}
                      fill="none"
                      stroke={learnListening ? "#5b8def" : "#34c759"}
                      strokeWidth={strokeW(4)}
                      style={{ pointerEvents: "none" }}
                    >
                      {learnListening && (
                        <animate
                          attributeName="opacity"
                          values="0.4;1;0.4"
                          dur="1.1s"
                          repeatCount="indefinite"
                        />
                      )}
                    </rect>
                  ) : (
                    <circle
                      cx={cx}
                      cy={cy}
                      r={r + selectionPad * 2}
                      fill="none"
                      stroke={learnListening ? "#5b8def" : "#34c759"}
                      strokeWidth={strokeW(4)}
                      style={{ pointerEvents: "none" }}
                    >
                      {learnListening && (
                        <animate
                          attributeName="opacity"
                          values="0.4;1;0.4"
                          dur="1.1s"
                          repeatCount="indefinite"
                        />
                      )}
                    </circle>
                  ))}
              </g>
            );
          })}
        </g>
      </svg>

      <Flex
        position="absolute"
        bottom={2}
        left={2}
        direction="column"
        gap={1}
        pointerEvents="none"
      >
        <HStack spacing={1} pointerEvents="auto">
          <Button size="xs" variant="solid" bg="kit.panel" onClick={() => nudgeZoom(1.15)}>
            +
          </Button>
          <Button size="xs" variant="solid" bg="kit.panel" onClick={() => nudgeZoom(1 / 1.15)}>
            −
          </Button>
          <Button size="xs" variant="outline" borderColor="kit.border" onClick={fitView}>
            Fit
          </Button>
        </HStack>
        <Text fontSize="10px" color="kit.textMuted" px={1}>
          Scroll to zoom · Drag to pan · Alt+drag to pan
        </Text>
      </Flex>

      <Box
        position="absolute"
        top={2}
        right={2}
        px={2}
        py={1}
        bg="blackAlpha.600"
        borderRadius="md"
        fontSize="xs"
        color="kit.textMuted"
      >
        {Math.round(viewport.zoom * 100)}%
      </Box>

      {menuPiece && (
        <Box
          position="fixed"
          left={`${menuPos.x}px`}
          top={`${menuPos.y}px`}
          zIndex={1000}
          bg="kit.panel"
          border="1px solid"
          borderColor="kit.border"
          borderRadius="md"
          boxShadow="lg"
          fontSize="sm"
          minW="160px"
          onClick={(e) => e.stopPropagation()}
          onPointerDown={(e) => e.stopPropagation()}
          onPointerUp={(e) => e.stopPropagation()}
        >
          {(
            [
              {
                label: "Learn MIDI Note",
                action: () => {
                  kitforgeBridge.learnMidi(menuPiece.id, menuArticulationId ?? "");
                },
              },
              {
                label: "Assign Sample",
                action: () => {
                  kitforgeBridge.assignSample(menuPiece.id, menuArticulationId ?? "");
                },
              },
              {
                label: "Swap Samples",
                action: () => {
                  const art =
                    menuPiece.articulations.find((a) => a.id === menuArticulationId) ??
                    menuPiece.articulations[0];
                  onSwapSamples({
                    pieceId: menuPiece.id,
                    articulationId: art?.id,
                    instrumentType: menuPiece.type,
                    articulationName: art?.name,
                    pieceName: menuPiece.name,
                    mode: "articulation",
                  });
                },
              },
              {
                label: "Arrange",
                submenu: [
                  { label: "Send to Front", action: () => kitforgeBridge.reorderPiece(menuPiece.id, "front") },
                  { label: "Send Forwards", action: () => kitforgeBridge.reorderPiece(menuPiece.id, "forward") },
                  { label: "Send Backwards", action: () => kitforgeBridge.reorderPiece(menuPiece.id, "backward") },
                  { label: "Send to Back", action: () => kitforgeBridge.reorderPiece(menuPiece.id, "back") },
                ],
              },
              {
                label: "Delete",
                action: () => kitforgeBridge.deletePiece(menuPiece.id),
                danger: true,
              },
            ] as {
              label: string;
              action?: () => void;
              danger?: boolean;
              submenu?: { label: string; action: () => void }[];
            }[]
          ).map((item) =>
            item.submenu ? (
              <Box
                key={item.label}
                position="relative"
                onMouseEnter={() => setOpenSubmenu(item.label)}
                onMouseLeave={() => setOpenSubmenu(null)}
              >
                <Flex
                  px={3}
                  py={2}
                  align="center"
                  justify="space-between"
                  cursor="default"
                  bg={openSubmenu === item.label ? "kit.border" : undefined}
                  _hover={{ bg: "kit.border" }}
                >
                  <Text as="span">{item.label}</Text>
                  <Text as="span" color="kit.textMuted" pl={3}>
                    ›
                  </Text>
                </Flex>
                {openSubmenu === item.label && (
                  <Box
                    position="absolute"
                    left="100%"
                    top={-1}
                    ml="-1px"
                    bg="kit.panel"
                    border="1px solid"
                    borderColor="kit.border"
                    borderRadius="md"
                    boxShadow="lg"
                    minW="160px"
                    zIndex={1001}
                  >
                    {item.submenu.map((sub) => (
                      <Box
                        key={sub.label}
                        px={3}
                        py={2}
                        cursor="pointer"
                        _hover={{ bg: "kit.border" }}
                        onClick={() => {
                          sub.action();
                          setMenuPiece(null);
                          setOpenSubmenu(null);
                        }}
                      >
                        {sub.label}
                      </Box>
                    ))}
                  </Box>
                )}
              </Box>
            ) : (
              <Box
                key={item.label}
                px={3}
                py={2}
                cursor="pointer"
                color={item.danger ? "red.300" : undefined}
                _hover={{ bg: "kit.border" }}
                onClick={() => {
                  item.action?.();
                  setMenuPiece(null);
                }}
              >
                {item.label}
              </Box>
            ),
          )}
        </Box>
      )}
    </Box>
  );
}
