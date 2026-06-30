import type { DrumPiece, DrumPieceType, KitModel } from "../../types/kit";

export const LAYOUT_REF_WIDTH = 980;
export const LAYOUT_REF_HEIGHT = 680;

export interface Viewport {
  zoom: number;
  panX: number;
  panY: number;
}

export interface PieceCircle {
  cx: number;
  cy: number;
  r: number;
}

export interface RefRect {
  minX: number;
  minY: number;
  maxX: number;
  maxY: number;
}

export function layoutReferenceSize(kit: KitModel): { refW: number; refH: number } {
  return {
    refW: kit.canvasWidth ?? LAYOUT_REF_WIDTH,
    refH: kit.canvasHeight ?? LAYOUT_REF_HEIGHT,
  };
}

/** UI kit state from JUCE uses normalized top-left x/y and width/height (0–1). */
export function pieceCircleGeometry(
  piece: DrumPiece,
  refW: number,
  refH: number,
): PieceCircle {
  const cx = (piece.x + piece.width * 0.5) * refW;
  const cy = (piece.y + piece.height * 0.5) * refH;
  const diameter = Math.max(piece.width * refW, piece.height * refH);
  return { cx, cy, r: diameter * 0.5 };
}

/** Axis-aligned bounds of one piece in layout reference pixels (matches SVG render). */
export function pieceRenderBounds(
  piece: DrumPiece,
  refW: number,
  refH: number,
): RefRect {
  const { cx, cy, r } = pieceCircleGeometry(piece, refW, refH);
  return {
    minX: cx - r,
    minY: cy - r,
    maxX: cx + r,
    maxY: cy + r,
  };
}

/** Union bounding box of all pieces in layout reference pixels. */
export function kitContentBounds(
  pieces: DrumPiece[],
  refW: number,
  refH: number,
): RefRect | null {
  if (pieces.length === 0) return null;

  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;

  for (const piece of pieces) {
    const b = pieceRenderBounds(piece, refW, refH);
    minX = Math.min(minX, b.minX);
    minY = Math.min(minY, b.minY);
    maxX = Math.max(maxX, b.maxX);
    maxY = Math.max(maxY, b.maxY);
  }

  if (!Number.isFinite(minX)) return null;

  return { minX, minY, maxX, maxY };
}

/** Fit view so `bounds` (plus padding) is centered in the viewport. */
export function fitViewportToBounds(
  bounds: RefRect,
  viewW: number,
  viewH: number,
  paddingPx = 48,
): Viewport {
  const padded = {
    minX: bounds.minX - paddingPx,
    minY: bounds.minY - paddingPx,
    maxX: bounds.maxX + paddingPx,
    maxY: bounds.maxY + paddingPx,
  };

  const contentW = Math.max(padded.maxX - padded.minX, 1);
  const contentH = Math.max(padded.maxY - padded.minY, 1);
  const zoom = clampZoom(Math.min(viewW / contentW, viewH / contentH));
  const cx = (padded.minX + padded.maxX) * 0.5;
  const cy = (padded.minY + padded.maxY) * 0.5;

  return {
    zoom,
    panX: viewW * 0.5 - cx * zoom,
    panY: viewH * 0.5 - cy * zoom,
  };
}

export function computeFitViewport(
  viewW: number,
  viewH: number,
  refW: number,
  refH: number,
  padding = 0.92,
): Viewport {
  const zoom = Math.min(viewW / refW, viewH / refH) * padding;
  return {
    zoom,
    panX: (viewW - refW * zoom) * 0.5,
    panY: (viewH - refH * zoom) * 0.5,
  };
}

/** Fit the viewport to the bounding box of all kit pieces (not the full layout canvas). */
export function computeFitViewportForKit(
  pieces: DrumPiece[],
  viewW: number,
  viewH: number,
  refW: number,
  refH: number,
  paddingPx = 48,
): Viewport {
  if (viewW <= 0 || viewH <= 0) {
    return { zoom: 1, panX: 0, panY: 0 };
  }

  const bounds = kitContentBounds(pieces, refW, refH);
  if (bounds == null) {
    return computeFitViewport(viewW, viewH, refW, refH);
  }

  return fitViewportToBounds(bounds, viewW, viewH, paddingPx);
}

export function clampZoom(zoom: number): number {
  return Math.min(4, Math.max(0.2, zoom));
}

/** Map pan/zoom state to an SVG viewBox (more reliable than `<g transform>` in embedded WebViews). */
export function viewportToViewBox(
  viewport: Viewport,
  viewW: number,
  viewH: number,
): { x: number; y: number; w: number; h: number } {
  const zoom = Math.max(viewport.zoom, 0.001);
  return {
    x: -viewport.panX / zoom,
    y: -viewport.panY / zoom,
    w: viewW / zoom,
    h: viewH / zoom,
  };
}

/** Screen coords → layout reference pixel coords. */
export function screenToRef(
  clientX: number,
  clientY: number,
  rect: DOMRect,
  viewport: Viewport,
): { x: number; y: number } {
  return {
    x: (clientX - rect.left - viewport.panX) / viewport.zoom,
    y: (clientY - rect.top - viewport.panY) / viewport.zoom,
  };
}

/** Screen coords → normalized center (0–1) in layout space. */
export function screenToNormalizedCenter(
  clientX: number,
  clientY: number,
  rect: DOMRect,
  viewport: Viewport,
  refW: number,
  refH: number,
): { x: number; y: number } {
  const ref = screenToRef(clientX, clientY, rect, viewport);
  return {
    x: ref.x / refW,
    y: ref.y / refH,
  };
}

export function zoomAtPoint(
  viewport: Viewport,
  pointerX: number,
  pointerY: number,
  rect: DOMRect,
  factor: number,
): Viewport {
  const newZoom = clampZoom(viewport.zoom * factor);
  const sx = pointerX - rect.left;
  const sy = pointerY - rect.top;
  return {
    zoom: newZoom,
    panX: sx - ((sx - viewport.panX) * newZoom) / viewport.zoom,
    panY: sy - ((sy - viewport.panY) * newZoom) / viewport.zoom,
  };
}

export function argbToCss(argb: number): string {
  return `#${(argb & 0xffffff).toString(16).padStart(6, "0")}`;
}

/** Paint order for top-down kit view. Lower = further back (drawn first). */
export function displayLayerOrder(type: DrumPieceType): number {
  switch (type) {
    case "kick":
      return 0;
    case "snare":
    case "rackTom":
    case "floorTom":
      return 10;
    case "ride":
    case "hiHat":
      return 20;
    case "splash":
    case "crash":
      return 30;
    case "china":
      return 40;
    case "accessory":
      return 25;
    default:
      return 10;
  }
}

/** Back → front for SVG paint order; preserves kit array order within the same layer. */
export function sortPiecesForDisplay(pieces: DrumPiece[]): DrumPiece[] {
  return pieces
    .map((piece, index) => ({ piece, index }))
    .sort((a, b) => {
      const orderA = displayLayerOrder(a.piece.type);
      const orderB = displayLayerOrder(b.piece.type);
      if (orderA !== orderB) return orderA - orderB;
      return a.index - b.index;
    })
    .map(({ piece }) => piece);
}

export function isKickPiece(piece: DrumPiece): boolean {
  return (
    piece.type === "kick" ||
    (piece.name.toLowerCase().includes("kick") && !isCymbalPiece(piece))
  );
}

export function isCymbalPiece(piece: DrumPiece): boolean {
  return (
    piece.type === "hiHat" ||
    piece.type === "crash" ||
    piece.type === "ride" ||
    piece.type === "china" ||
    piece.type === "splash" ||
    piece.shapeType === "cymbal" ||
    piece.shapeType === "oval"
  );
}

export function isRidePiece(piece: DrumPiece): boolean {
  return piece.type === "ride" || piece.name.toLowerCase().includes("ride");
}

export function findArticulationByName(
  piece: DrumPiece,
  name: string,
): DrumPiece["articulations"][number] | undefined {
  const target = name.toLowerCase();
  return piece.articulations.find((art) => art.name.toLowerCase() === target);
}

export function rideEdgeArticulation(piece: DrumPiece) {
  return (
    findArticulationByName(piece, "Edge") ??
    findArticulationByName(piece, "Bow") ??
    findArticulationByName(piece, "Ride") ??
    findArticulationByName(piece, "Hit") ??
    piece.articulations.find((art) => art.midiNote === 51) ??
    piece.articulations[0]
  );
}

/** @deprecated Use rideEdgeArticulation */
export function rideBowArticulation(piece: DrumPiece) {
  return rideEdgeArticulation(piece);
}

export function rideBellArticulation(piece: DrumPiece) {
  return findArticulationByName(piece, "Bell");
}

/** Ride bell radius as fraction of piece radius (visual). */
export const RIDE_BELL_R = 0.3;
/** Slightly smaller hit target so edge clicks don't bleed into the bell zone. */
export const RIDE_BELL_HIT_R = 0.26;
/** Edge MIDI label sits in the annulus between bell and rim. */
export const RIDE_EDGE_MIDI_Y = 0.72;

export type RideHitZone = "bell" | "edge";

export type RideArticulationRef = { id: string; name: string };

/** Resolves ride Edge vs Bell from a canvas hit; returns id and name together so
 * selection stays correct even when both articulations share the same id in the model. */
export function rideArticulationRefFromPoint(
  piece: DrumPiece,
  clientX: number,
  clientY: number,
  rect: DOMRect,
  viewport: Viewport,
  refW: number,
  refH: number,
): RideArticulationRef | null {
  const edgeArt = rideEdgeArticulation(piece);
  const bellArt = rideBellArticulation(piece);
  if (!edgeArt) return null;

  const { cx, cy, r } = pieceCircleGeometry(piece, refW, refH);
  const svgX = (clientX - rect.left - viewport.panX) / viewport.zoom;
  const svgY = (clientY - rect.top - viewport.panY) / viewport.zoom;
  const dist = Math.hypot(svgX - cx, svgY - cy);

  if (dist > r) return null;

  if (bellArt != null && dist <= r * RIDE_BELL_HIT_R) {
    return { id: bellArt.id, name: bellArt.name };
  }
  return { id: edgeArt.id, name: edgeArt.name };
}

export function rideArticulationFromPoint(
  piece: DrumPiece,
  clientX: number,
  clientY: number,
  rect: DOMRect,
  viewport: Viewport,
  refW: number,
  refH: number,
): string | null {
  return rideArticulationRefFromPoint(
    piece,
    clientX,
    clientY,
    rect,
    viewport,
    refW,
    refH,
  )?.id ?? null;
}
