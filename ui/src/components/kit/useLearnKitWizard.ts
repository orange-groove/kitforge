import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { KitModel } from "../../types/kit";
import { kitforgeBridge, subscribe } from "../../bridge/kitforgeBridge";

export interface LearnStep {
  pieceId: string;
  pieceName: string;
  /** Empty string targets the piece's primary articulation. */
  articulationId: string;
  articulationName: string;
  /** True when the parent piece exposes more than one articulation. */
  multiArticulation: boolean;
}

export interface LearnKitWizard {
  active: boolean;
  finished: boolean;
  steps: LearnStep[];
  currentIndex: number;
  currentStep: LearnStep | null;
  /** Piece currently listening for a note (blue highlight). */
  activePieceId: string | null;
  /** Pieces whose articulations have all been learned/skipped (green highlight). */
  donePieceIds: Set<string>;
  start: () => void;
  skip: () => void;
  stop: () => void;
}

function buildSteps(kit: KitModel): LearnStep[] {
  const steps: LearnStep[] = [];
  for (const piece of kit.pieces) {
    if (piece.articulations.length === 0) {
      steps.push({
        pieceId: piece.id,
        pieceName: piece.name,
        articulationId: "",
        articulationName: "Hit",
        multiArticulation: false,
      });
      continue;
    }
    const multi = piece.articulations.length > 1;
    for (const art of piece.articulations) {
      steps.push({
        pieceId: piece.id,
        pieceName: piece.name,
        articulationId: art.id,
        articulationName: art.name,
        multiArticulation: multi,
      });
    }
  }
  return steps;
}

/**
 * Drives the "Learn My Kit" flow: arms MIDI-learn for one articulation at a
 * time, advances when the native side reports a learned note, and exposes
 * per-piece highlight state for the canvas.
 */
export function useLearnKitWizard(kit: KitModel): LearnKitWizard {
  const [active, setActive] = useState(false);
  const [steps, setSteps] = useState<LearnStep[]>([]);
  const [currentIndex, setCurrentIndex] = useState(0);

  const activeRef = useRef(active);
  const stepsRef = useRef(steps);
  const indexRef = useRef(currentIndex);
  activeRef.current = active;
  stepsRef.current = steps;
  indexRef.current = currentIndex;

  const arm = useCallback((step: LearnStep) => {
    kitforgeBridge.learnMidi(step.pieceId, step.articulationId);
  }, []);

  const advance = useCallback(() => {
    const next = indexRef.current + 1;
    setCurrentIndex(next);
    if (next >= stepsRef.current.length) {
      // Finished: make sure the engine isn't left armed on the last target.
      kitforgeBridge.cancelLearnMidi();
      return;
    }
    arm(stepsRef.current[next]);
  }, [arm]);

  const start = useCallback(() => {
    const built = buildSteps(kit);
    if (built.length === 0) return;
    setSteps(built);
    setCurrentIndex(0);
    setActive(true);
    arm(built[0]);
  }, [kit, arm]);

  const skip = useCallback(() => {
    if (!activeRef.current) return;
    advance();
  }, [advance]);

  const stop = useCallback(() => {
    setActive(false);
    setSteps([]);
    setCurrentIndex(0);
    kitforgeBridge.cancelLearnMidi();
  }, []);

  useEffect(() => {
    const unsub = subscribe((msg) => {
      if (msg.type !== "midiLearnCompleted") return;
      if (!activeRef.current) return;
      const step = stepsRef.current[indexRef.current];
      if (!step) return;
      if (msg.pieceId === step.pieceId && msg.articulationId === step.articulationId) {
        advance();
      }
    });
    return unsub;
  }, [advance]);

  const finished = active && steps.length > 0 && currentIndex >= steps.length;
  const currentStep = active && currentIndex < steps.length ? steps[currentIndex] : null;
  const activePieceId = currentStep?.pieceId ?? null;

  const donePieceIds = useMemo(() => {
    const done = new Set<string>();
    if (!active) return done;
    const lastIndexByPiece = new Map<string, number>();
    steps.forEach((s, i) => lastIndexByPiece.set(s.pieceId, i));
    lastIndexByPiece.forEach((lastIdx, pieceId) => {
      if (currentIndex > lastIdx) done.add(pieceId);
    });
    return done;
  }, [active, steps, currentIndex]);

  return {
    active,
    finished,
    steps,
    currentIndex,
    currentStep,
    activePieceId,
    donePieceIds,
    start,
    skip,
    stop,
  };
}
