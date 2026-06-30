import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Box, Flex, Text } from "@chakra-ui/react";
import type { DrumPiece, KitModel, SwapTarget } from "../../types/kit";
import { kitforgeBridge, subscribe } from "../../bridge/kitforgeBridge";
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
  rideArticulationRefFromPoint,
  RIDE_BELL_R,
  RIDE_BELL_HIT_R,
  RIDE_EDGE_MIDI_Y,
  type RideHitZone,
  screenToNormalizedCenter,
  sortPiecesForDisplay,
  type Viewport,
  viewportToViewBox,
  zoomAtPoint,
} from "./layoutUtils";
import { CanvasViewportControls } from "./CanvasViewportControls";
interface DrumCanvasProps {
  kit: KitModel;
  editLayout: boolean;
  onSelectPiece: (
    pieceId: string | null,
    articulationId?: string | null,
    articulationName?: string | null,
  ) => void;
  onSwapSamples: (target: SwapTarget) => void;
  /** Piece currently listening in the Learn My Kit wizard (blue highlight). */
  learnActivePieceId?: string | null;
  /** Pieces already learned in the wizard (green highlight). */
  learnDonePieceIds?: Set<string>;
}

const CLICK_SLOP_PX = 6;

const DRUM_RIM_COLOR = "#231f20";

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

  const kitRef = useRef(kit);
  kitRef.current = kit;

  const panDrag = useRef<{ startX: number; startY: number; panX: number; panY: number } | null>(null);
  const pendingInteraction = useRef<PendingInteraction | null>(null);
  const userAdjustedViewRef = useRef(false);
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
  // Bumped on every hit; used as the React key of the cymbal group so its
  // one-shot wobble animation replays from the start on each strike.
  const [wobbleNonce, setWobbleNonce] = useState<Record<string, number>>({});

  const flashHit = useCallback((pieceId: string) => {
    setHitPieceIds((prev) => {
      const next = new Set(prev);
      next.add(pieceId);
      return next;
    });
    setWobbleNonce((prev) => ({ ...prev, [pieceId]: (prev[pieceId] ?? 0) + 1 }));
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

  // Flash/wobble pieces hit via live MIDI (the C++ engine reports each strike).
  useEffect(() => {
    return subscribe((msg) => {
      if (msg.type === "pieceHit") flashHit(msg.pieceId);
    });
  }, [flashHit]);

  const [menuPiece, setMenuPiece] = useState<DrumPiece | null>(null);
  const [menuArticulationId, setMenuArticulationId] = useState<string | null>(null);
  const [openSubmenu, setOpenSubmenu] = useState<string | null>(null);
  const [menuPos, setMenuPos] = useState({ x: 0, y: 0 });

  const { refW, refH } = layoutReferenceSize(kit);
  const displayPieces = useMemo(() => sortPiecesForDisplay(kit.pieces), [kit.pieces]);
  const layoutRef = useRef({ refW, refH });
  layoutRef.current = { refW, refH };

  const sizeRef = useRef(size);
  sizeRef.current = size;

  const runFit = useCallback(() => {
    const el = containerRef.current;
    if (!el) return;

    const w = sizeRef.current.w > 0 ? sizeRef.current.w : el.clientWidth;
    const h = sizeRef.current.h > 0 ? sizeRef.current.h : el.clientHeight;
    if (w <= 0 || h <= 0) return;

    const { refW: rw, refH: rh } = layoutRef.current;
    const pieces = kitRef.current.pieces.map((p) => pieceForDisplay(p));
    const next = computeFitViewportForKit(pieces, w, h, rw, rh);

    userAdjustedViewRef.current = false;
    viewportRef.current = next;
    setViewport(next);
  }, [pieceForDisplay]);

  const runFitRef = useRef(runFit);
  runFitRef.current = runFit;

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const ro = new ResizeObserver((entries) => {
      const cr = entries[0]?.contentRect;
      if (cr && cr.width > 0 && cr.height > 0) {
        setSize({ w: cr.width, h: cr.height });
      }
    });
    ro.observe(el);
    // Initial measure — ResizeObserver can fire late in embedded WebViews.
    const rect = el.getBoundingClientRect();
    if (rect.width > 0 && rect.height > 0) {
      setSize({ w: rect.width, h: rect.height });
    }
    return () => ro.disconnect();
  }, []);

  const lastAutoFitKey = useRef("");

  useEffect(() => {
    const key = `${kit.kitName}|${kit.pieces.length}|${size.w}|${size.h}|${refW}|${refH}`;
    if (key === lastAutoFitKey.current) return;
    lastAutoFitKey.current = key;
    runFitRef.current();
  }, [kit.kitName, kit.pieces.length, size.w, size.h, refW, refH]);

  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === "0" && !e.metaKey && !e.ctrlKey && !e.altKey) {
        e.preventDefault();
        runFitRef.current();
      }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, []);

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      userAdjustedViewRef.current = true;
      const rect = el.getBoundingClientRect();
      const factor = e.deltaY > 0 ? 0.9 : 1.1;
      setViewport((v) => zoomAtPoint(v, e.clientX, e.clientY, rect, factor));
    };

    el.addEventListener("wheel", onWheel, { passive: false });
    return () => el.removeEventListener("wheel", onWheel);
  }, []);

  const nudgeZoom = useCallback((factor: number) => {
    userAdjustedViewRef.current = true;
    const el = containerRef.current;
    if (!el) return;
    const rect = el.getBoundingClientRect();
    const cx = rect.left + rect.width * 0.5;
    const cy = rect.top + rect.height * 0.5;
    setViewport((v) => zoomAtPoint(v, cx, cy, rect, factor));
  }, []);

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

  const resolveRideArticulation = (
    piece: DrumPiece,
    target: EventTarget | null,
    clientX: number,
    clientY: number,
    refWidth: number,
    refHeight: number,
  ) => {
    const edgeArt = rideEdgeArticulation(piece);
    const bellArt = rideBellArticulation(piece);
    if (!edgeArt) return null;

    const zone = findRideZoneFromTarget(target);
    if (zone === "bell" && bellArt != null) {
      return { id: bellArt.id, name: bellArt.name };
    }
    if (zone === "edge") {
      return { id: edgeArt.id, name: edgeArt.name };
    }

    const rect = containerRef.current?.getBoundingClientRect();
    if (!rect) {
      return { id: edgeArt.id, name: edgeArt.name };
    }

    return (
      rideArticulationRefFromPoint(
        piece,
        clientX,
        clientY,
        rect,
        viewportRef.current,
        refWidth,
        refHeight,
      ) ?? { id: edgeArt.id, name: edgeArt.name }
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
        const art = resolveRideArticulation(
          piece,
          target,
          downX,
          downY,
          refWidth,
          refHeight,
        );
        if (!art) return;

        onSelectPiece(piece.id, art.id, art.name);
        flashHit(piece.id);
        kitforgeBridge.triggerPiece(piece.id, art.id);
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
      const art = resolveRideArticulation(
        piece,
        e.target,
        e.clientX,
        e.clientY,
        refW,
        refH,
      );
      if (art) {
        onSelectPiece(piece.id, art.id, art.name);
        flashHit(piece.id);
        kitforgeBridge.triggerPiece(piece.id, art.id);
        const ridePieceId = piece.id;
        const rideArt = art;
        onClick = () => {
          onSelectPiece(ridePieceId, rideArt.id, rideArt.name);
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
      userAdjustedViewRef.current = true;
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

  const viewBox = useMemo(
    () => viewportToViewBox(viewport, size.w, size.h),
    [viewport, size.w, size.h],
  );

  return (
    <Box
      flex="1"
      position="relative"
      minH={0}
      minW={0}
      borderRadius="md"
      border="1px solid"
      borderColor="kit.border"
      overflow="hidden"
    >
      <Box
        ref={containerRef}
        position="absolute"
        inset={0}
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
        viewBox={`${viewBox.x} ${viewBox.y} ${viewBox.w} ${viewBox.h}`}
        preserveAspectRatio="none"
        style={{ display: "block", shapeRendering: "geometricPrecision", flexShrink: 0 }}
      >
        <defs>
          <radialGradient
            id="kitforge-canvas-bg"
            gradientUnits="userSpaceOnUse"
            cx={viewBox.x + viewBox.w * 0.5}
            cy={viewBox.y + viewBox.h * 0.38}
            r={Math.max(viewBox.w, viewBox.h) * 0.95}
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
          <style>{`
            @keyframes kitforge-cymbal-wobble {
              0%   { transform: skewX(0deg); }
              15%  { transform: skewX(-1.5deg); }
              35%  { transform: skewX(1deg); }
              55%  { transform: skewX(-0.6deg); }
              75%  { transform: skewX(0.3deg); }
              100% { transform: skewX(0deg); }
            }
            .kitforge-cymbal-wobble {
              transform-box: fill-box;
              transform-origin: center;
              animation: kitforge-cymbal-wobble 0.45s ease-out both;
            }
          `}</style>
        </defs>

        <rect x={viewBox.x} y={viewBox.y} width={viewBox.w} height={viewBox.h} fill="url(#kitforge-canvas-bg)" />

          {displayPieces.map((piece) => {
            const displayPiece = pieceForDisplay(piece);
            const { cx, cy, r } = pieceCircleGeometry(displayPiece, refW, refH);
            const hit = hitPieceIds.has(piece.id);
            const wob = wobbleNonce[piece.id] ?? 0;
            const learnListening = learnActivePieceId === piece.id;
            const learnDone = learnDonePieceIds?.has(piece.id) ?? false;
            const cymbal = isCymbalPiece(piece);
            const kick = isKickPiece(piece);
            const isRide = isRidePiece(piece);
            const edgeArt = isRide ? rideEdgeArticulation(piece) : undefined;
            const bellArt = isRide ? rideBellArticulation(piece) : undefined;
            const fill = argbToCss(piece.color);
            const labelColor = cymbal ? "#f0f0f0" : "#111111";
            const strokeW = (w: number) => w / viewport.zoom;
            const selectionPad = strokeW(2);
            const edgeMidi = edgeArt?.midiNote ?? piece.primaryMidiNote;

            const openContextMenu = (
              e: React.MouseEvent,
              articulationId?: string,
              articulationName?: string,
            ) => {
              e.preventDefault();
              e.stopPropagation();
              onSelectPiece(piece.id, articulationId ?? null, articulationName ?? null);
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

              const art =
                rideArticulationRefFromPoint(
                  piece,
                  clientX,
                  clientY,
                  rect,
                  viewportRef.current,
                  refW,
                  refH,
                ) ?? { id: edgeArt.id, name: edgeArt.name };

              onSelectPiece(piece.id, art.id, art.name);
              setMenuPiece(piece);
              setMenuArticulationId(art.id);
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
                    rimFill={DRUM_RIM_COLOR}
                  />
                ) : (
                  <g
                    key={`wobble-${wob}`}
                    className={wob > 0 ? "kitforge-cymbal-wobble" : undefined}
                  >
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
                  </g>
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
      </svg>
      </Box>

      <CanvasViewportControls
        zoomPercent={Math.round(viewport.zoom * 100)}
        onZoomIn={() => nudgeZoom(1.15)}
        onZoomOut={() => nudgeZoom(1 / 1.15)}
        onFit={() => runFitRef.current()}
      />

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
