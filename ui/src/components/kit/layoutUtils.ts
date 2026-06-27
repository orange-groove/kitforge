import type { DrumPiece, KitModel } from "../../types/kit";

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

export function layoutReferenceSize(kit: KitModel): { refW: number; refH: number } {
  return {
    refW: kit.canvasWidth ?? LAYOUT_REF_WIDTH,
    refH: kit.canvasHeight ?? LAYOUT_REF_HEIGHT,
  };
}

/** Circle geometry in layout reference pixels (always round). */
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
  padding = 0.9,
): Viewport {
  if (pieces.length === 0 || viewW <= 0 || viewH <= 0) {
    return computeFitViewport(viewW, viewH, refW, refH, padding);
  }

  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;

  for (const piece of pieces) {
    const { cx, cy, r } = pieceCircleGeometry(piece, refW, refH);
    minX = Math.min(minX, cx - r);
    minY = Math.min(minY, cy - r);
    maxX = Math.max(maxX, cx + r);
    maxY = Math.max(maxY, cy + r);
  }

  const contentW = Math.max(maxX - minX, 40);
  const contentH = Math.max(maxY - minY, 40);
  const zoom = clampZoom(Math.min(viewW / contentW, viewH / contentH) * padding);
  const contentCx = (minX + maxX) * 0.5;
  const contentCy = (minY + maxY) * 0.5;

  return {
    zoom,
    panX: viewW * 0.5 - contentCx * zoom,
    panY: viewH * 0.5 - contentCy * zoom,
  };
}

export function clampZoom(zoom: number): number {
  return Math.min(4, Math.max(0.2, zoom));
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

export function rideArticulationFromPoint(
  piece: DrumPiece,
  clientX: number,
  clientY: number,
  rect: DOMRect,
  viewport: Viewport,
  refW: number,
  refH: number,
): string | null {
  const edgeArt = rideEdgeArticulation(piece);
  const bellArt = rideBellArticulation(piece);
  if (!edgeArt) return null;

  const { cx, cy, r } = pieceCircleGeometry(piece, refW, refH);
  const svgX = (clientX - rect.left - viewport.panX) / viewport.zoom;
  const svgY = (clientY - rect.top - viewport.panY) / viewport.zoom;
  const dist = Math.hypot(svgX - cx, svgY - cy);

  if (dist > r) return null;

  if (bellArt != null && dist <= r * RIDE_BELL_HIT_R) return bellArt.id;
  return edgeArt.id;
}
