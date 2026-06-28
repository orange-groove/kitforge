export type DrumPieceType =
  | "kick"
  | "snare"
  | "rackTom"
  | "floorTom"
  | "hiHat"
  | "crash"
  | "ride"
  | "china"
  | "splash"
  | "accessory";

export type ShapeType = "circle" | "rectangle" | "oval" | "cymbal" | "polygon";

export interface DrumSample {
  id: string;
  filePath: string;
  rootMidiNote: number;
  gain: number;
  pan: number;
  pitch: number;
  startOffsetSamples: number;
  endOffsetSamples: number;
}

export interface SampleLayer {
  id: string;
  minVelocity: number;
  maxVelocity: number;
  currentRoundRobinIndex: number;
  roundRobins: DrumSample[];
}

export interface Articulation {
  id: string;
  name: string;
  midiNote: number;
  chokeGroupId: string;
  layers: SampleLayer[];
  /** Present in UI kit state from C++; avoids serializing sample lists. */
  hasSample?: boolean;
}

export interface DrumPiece {
  id: string;
  name: string;
  type: DrumPieceType;
  midiNotes: number[];
  primaryMidiNote: number;
  chokeGroupId: string;
  outputChannelPair: number;
  volume: number;
  pan: number;
  pitch: number;
  pitchSemitones?: number;
  muted: boolean;
  soloed: boolean;
  x: number;
  y: number;
  width: number;
  height: number;
  rotation: number;
  color: number;
  shapeType: ShapeType;
  articulations: Articulation[];
}

export interface KitModel {
  version: number;
  kitName: string;
  canvasWidth?: number;
  canvasHeight?: number;
  pieces: DrumPiece[];
}

export interface InstalledKit {
  id: string;
  name: string;
  format: string;
  license: string;
  version: string;
  author: string;
  installedPath: string;
  sampleCount: number;
  pieceCount: number;
  tags: string[];
  thumbnailPath?: string;
  previewAudioPath?: string;
}

export interface LibraryMappingSource {
  key: string;
  pieceName: string;
  articulationName: string;
  type: DrumPieceType;
  midiNote: number;
  sampleCount: number;
}

export interface LibraryMappingTarget {
  key: string;
  label: string;
  pieceId: string;
  articulationId: string;
}

export interface LibraryMapping {
  sourceKey: string;
  targetKey: string;
  enabled: boolean;
}

export interface LibraryMappingState {
  libraryId: string;
  libraryName: string;
  kitName: string;
  sources: LibraryMappingSource[];
  targets: LibraryMappingTarget[];
  mappings: LibraryMapping[];
}

// ---- Cross-library sample swapping ----

export type SwapMode = "articulation" | "piece" | "layer";

/** A swappable musical unit indexed from an installed library (UI summary). */
export interface SampleSetSummary {
  id: string;
  libraryId: string;
  displayName: string;
  instrumentType: DrumPieceType;
  articulation: string;
  tags: string[];
  sampleCount: number;
  velocityLayerCount: number;
  roundRobinCount: number;
  sourceKitName: string;
  sourcePackPath: string;
  allSamplesPresent: boolean;
}

export interface SampleIndexState {
  sampleSetCount: number;
  libraryCount: number;
  missingSampleCount: number;
}

export interface SampleSetQuery {
  instrumentType?: string;
  articulation?: string;
  libraryId?: string;
  text?: string;
  tags?: string[];
  minVelocityLayers?: number;
  minRoundRobins?: number;
}

export interface SwapTarget {
  pieceId: string;
  articulationId?: string;
  layerId?: string;
  instrumentType: DrumPieceType;
  articulationName?: string;
  pieceName?: string;
  mode: SwapMode;
}

export interface SwapOptions {
  useSourceName?: boolean;
  useSourceMidi?: boolean;
  useSourceVelocityRanges?: boolean;
}

export interface ValidationReport {
  errors: string[];
  warnings: string[];
}

export type NativeMessage =
  | { type: "kitState"; kit: KitModel }
  | { type: "catalogState"; installed: InstalledKit[] }
  | { type: "kitInstalled"; kitId: string; kitName: string; message: string }
  | { type: "kitSaved"; message: string }
  | { type: "kitRemoved"; kitId: string }
  | { type: "midiLearnStarted"; pieceId: string; articulationId: string }
  | { type: "midiLearnCompleted"; pieceId: string; articulationId: string; midiNote: number }
  | { type: "sampleAssigned"; pieceId: string; articulationId: string; fileName: string }
  | { type: "libraryMappingState"; libraryId: string; libraryName: string; kitName: string; sources: LibraryMappingSource[]; targets: LibraryMappingTarget[]; mappings: LibraryMapping[] }
  | { type: "libraryApplied"; libraryId: string; libraryName: string; appliedCount: number; message: string }
  | { type: "aiBuildComplete"; success: boolean; message: string }
  | { type: "sampleSetSearchResults"; results: SampleSetSummary[] }
  | { type: "sampleIndexState"; sampleSetCount: number; libraryCount: number; missingSampleCount: number }
  | { type: "sampleSwapCompleted"; pieceId: string; articulationId: string; sampleSetId: string }
  | { type: "sampleSwapFailed"; message: string }
  | { type: "validationState"; report: ValidationReport }
  | { type: "busy"; label: string }
  | { type: "error"; message: string };

export type OutboundMessage =
  | { type: "ready" }
  | { type: "triggerPiece"; pieceId: string; articulationId?: string }
  | { type: "movePiece"; pieceId: string; x: number; y: number; finalize?: boolean }
  | { type: "resizePiece"; pieceId: string; width: number; height: number }
  | { type: "learnMidi"; pieceId: string; articulationId: string }
  | { type: "cancelLearnMidi" }
  | { type: "assignSample"; pieceId: string; articulationId: string }
  | { type: "updatePiece"; pieceId: string; volume?: number; pan?: number; pitch?: number; muted?: boolean; soloed?: boolean }
  | { type: "renamePiece"; pieceId: string; name: string }
  | { type: "setArticulationMidi"; pieceId: string; articulationId: string; midiNote: number }
  | { type: "reorderPiece"; pieceId: string; mode: "front" | "back" | "forward" | "backward" }
  | { type: "deletePiece"; pieceId: string }
  | { type: "resizeEditor"; width: number; height: number }
  | { type: "saveKit" }
  | { type: "saveKitAs" }
  | { type: "loadKit" }
  | { type: "importSfz" }
  | { type: "importLooseFolder" }
  | { type: "installKitforge" }
  | { type: "removeKit"; kitId: string }
  | { type: "loadInstalledKit"; kitId: string }
  | { type: "revealKit"; kitId: string }
  | { type: "useLibrary"; libraryId: string }
  | { type: "getLibraryMapping"; libraryId: string }
  | { type: "applyLibraryMapping"; libraryId: string; mappings: LibraryMapping[] }
  | { type: "searchSampleSets"; query: SampleSetQuery }
  | { type: "previewSampleSet"; sampleSetId: string }
  | {
      type: "swapSampleSet";
      target: { pieceId: string; articulationId?: string; layerId?: string; mode: SwapMode };
      sampleSetId: string;
      options?: SwapOptions;
    }
  | { type: "rebuildSampleIndex" }
  | { type: "aiBuildKit"; prompt: string };

/** @deprecated Use InstalledKit */
export type InstalledLibrary = InstalledKit;
