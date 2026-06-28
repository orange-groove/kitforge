import type {
  NativeMessage,
  OutboundMessage,
  LibraryMapping,
  SampleSetQuery,
  SwapMode,
  SwapOptions,
} from "../types/kit";
import { getNativeFunction, isJuceBackendAvailable } from "./juceShim";

const NATIVE_FN = "kitforgeMessage";
const NATIVE_EVENT = "kitforgeFromNative";

declare global {
  interface Window {
    __JUCE__?: {
      backend: {
        emitEvent: (eventId: string, payload: unknown) => void;
        addEventListener: (eventId: string, fn: (payload: unknown) => void) => number;
        removeEventListener: (token: number) => void;
      };
    };
  }
}

type Listener = (message: NativeMessage) => void;

let listeners: Listener[] = [];
let eventToken: number | null = null;

function sendRaw(message: OutboundMessage): void {
  const json = JSON.stringify(message);

  if (isJuceBackendAvailable()) {
    void getNativeFunction(NATIVE_FN)(json);
    return;
  }

  console.debug("[kitforge mock]", message);
}

export function isJuceAvailable(): boolean {
  return isJuceBackendAvailable();
}

export function subscribe(listener: Listener): () => void {
  listeners = [...listeners, listener];

  if (isJuceBackendAvailable() && eventToken == null) {
    eventToken = window.__JUCE__!.backend.addEventListener(NATIVE_EVENT, (payload) => {
      const msg = payload as NativeMessage;
      listeners.forEach((l) => l(msg));
    });
  }

  return () => {
    listeners = listeners.filter((l) => l !== listener);
  };
}

export function sendReady(): void {
  sendRaw({ type: "ready" });
}

export const kitforgeBridge = {
  triggerPiece: (pieceId: string, articulationId?: string) =>
    sendRaw({ type: "triggerPiece", pieceId, ...(articulationId ? { articulationId } : {}) }),
  movePiece: (pieceId: string, x: number, y: number, finalize?: boolean) =>
    sendRaw({
      type: "movePiece",
      pieceId,
      x,
      y,
      ...(finalize ? { finalize: true } : {}),
    }),
  resizePiece: (pieceId: string, width: number, height: number) =>
    sendRaw({ type: "resizePiece", pieceId, width, height }),
  learnMidi: (pieceId: string, articulationId: string) =>
    sendRaw({ type: "learnMidi", pieceId, articulationId }),
  cancelLearnMidi: () => sendRaw({ type: "cancelLearnMidi" }),
  assignSample: (pieceId: string, articulationId: string) =>
    sendRaw({ type: "assignSample", pieceId, articulationId }),
  updatePiece: (
    pieceId: string,
    fields: {
      volume?: number;
      pan?: number;
      pitch?: number;
      muted?: boolean;
      soloed?: boolean;
    },
  ) => sendRaw({ type: "updatePiece", pieceId, ...fields }),
  renamePiece: (pieceId: string, name: string) =>
    sendRaw({ type: "renamePiece", pieceId, name }),
  setArticulationMidi: (pieceId: string, articulationId: string, midiNote: number) =>
    sendRaw({ type: "setArticulationMidi", pieceId, articulationId, midiNote }),
  reorderPiece: (
    pieceId: string,
    mode: "front" | "back" | "forward" | "backward",
  ) => sendRaw({ type: "reorderPiece", pieceId, mode }),
  deletePiece: (pieceId: string) => sendRaw({ type: "deletePiece", pieceId }),
  resizeEditor: (width: number, height: number) =>
    sendRaw({ type: "resizeEditor", width: Math.round(width), height: Math.round(height) }),
  saveKit: () => sendRaw({ type: "saveKit" }),
  saveKitAs: () => sendRaw({ type: "saveKitAs" }),
  loadKit: () => sendRaw({ type: "loadKit" }),
  importSfz: () => sendRaw({ type: "importSfz" }),
  importLooseFolder: () => sendRaw({ type: "importLooseFolder" }),
  installKitforge: () => sendRaw({ type: "installKitforge" }),
  removeKit: (kitId: string) => sendRaw({ type: "removeKit", kitId }),
  loadInstalledKit: (kitId: string) => sendRaw({ type: "loadInstalledKit", kitId }),
  revealKit: (kitId: string) => sendRaw({ type: "revealKit", kitId }),
  useLibrary: (libraryId: string) => sendRaw({ type: "useLibrary", libraryId }),
  getLibraryMapping: (libraryId: string) => sendRaw({ type: "getLibraryMapping", libraryId }),
  applyLibraryMapping: (libraryId: string, mappings: LibraryMapping[]) =>
    sendRaw({ type: "applyLibraryMapping", libraryId, mappings }),
  searchSampleSets: (query: SampleSetQuery) =>
    sendRaw({ type: "searchSampleSets", query }),
  previewSampleSet: (sampleSetId: string) =>
    sendRaw({ type: "previewSampleSet", sampleSetId }),
  swapSampleSet: (
    target: { pieceId: string; articulationId?: string; layerId?: string; mode: SwapMode },
    sampleSetId: string,
    options?: SwapOptions,
  ) => sendRaw({ type: "swapSampleSet", target, sampleSetId, ...(options ? { options } : {}) }),
  rebuildSampleIndex: () => sendRaw({ type: "rebuildSampleIndex" }),
  aiBuildKit: (prompt: string) => sendRaw({ type: "aiBuildKit", prompt }),
};

export function pitchRatioFromSemitones(semitones: number): number {
  return Math.pow(2, semitones / 12);
}

export function semitonesFromPitchRatio(ratio: number): number {
  return 12 * Math.log2(Math.max(ratio, 0.001));
}
