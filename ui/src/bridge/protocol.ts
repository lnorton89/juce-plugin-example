export const protocolVersion = 1 as const;
export const uiReadyEvent = 'ui.ready' as const;
export const hostInfoEvent = 'host.info' as const;
export const bridgeErrorEvent = 'bridge.error' as const;
export const spectrumSnapshotEvent = 'spectrum.snapshot' as const;
export const maxSpectrumBins = 256 as const;

export interface UiReady { protocolVersion: typeof protocolVersion }
export interface HostInfo {
  protocolVersion: typeof protocolVersion;
  productName: 'LumaScope';
  companyName: 'Signal Foundry Audio';
  productVersion: string;
  hostMode: 'VST3' | 'Standalone';
  uiSource: 'embedded' | 'vite';
  buildMarker: string;
}
export interface BridgeError { code: string; message: string; protocolVersion: number }

export type AnalyzerProfile = 'Musical' | 'Measurement' | 'Fast';

export interface SpectrumBin {
  frequencyHz: number;
  decibels: number;
  normalisedValue: number;
}

export interface SpectrumSnapshot {
  protocolVersion: typeof protocolVersion;
  sequence: number;
  profile: AnalyzerProfile;
  sampleRate: number;
  fftSize: number;
  minFrequencyHz: number;
  maxFrequencyHz: number;
  minDecibels: number;
  maxDecibels: number;
  bins: SpectrumBin[];
}

export type BridgeStatus =
  | { state: 'connecting'; spectrumSnapshot?: SpectrumSnapshot }
  | { state: 'ready'; hostInfo: HostInfo; spectrumSnapshot?: SpectrumSnapshot }
  | { state: 'error'; code: string; message: string; spectrumSnapshot?: SpectrumSnapshot };

export function parseHostInfo(value: unknown): HostInfo | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  const bounded = (field: unknown) => typeof field === 'string' && field.length > 0 && field.length <= 128;
  return item.protocolVersion === protocolVersion && item.productName === 'LumaScope'
    && item.companyName === 'Signal Foundry Audio' && bounded(item.productVersion)
    && bounded(item.buildMarker)
    && (item.hostMode === 'VST3' || item.hostMode === 'Standalone')
    && (item.uiSource === 'embedded' || item.uiSource === 'vite') ? item as unknown as HostInfo : null;
}

export function parseBridgeError(value: unknown): BridgeError | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  return typeof item.code === 'string' && item.code.length <= 64
    && typeof item.message === 'string' && item.message.length <= 256
    && typeof item.protocolVersion === 'number' ? item as unknown as BridgeError : null;
}

function isFiniteNumber(value: unknown): value is number {
  return typeof value === 'number' && Number.isFinite(value);
}

function isPositiveInteger(value: unknown): value is number {
  return Number.isSafeInteger(value) && Number(value) > 0;
}

function isProfile(value: unknown): value is AnalyzerProfile {
  return value === 'Musical' || value === 'Measurement' || value === 'Fast';
}

function parseSpectrumBin(value: unknown): SpectrumBin | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (!isFiniteNumber(item.frequencyHz) || item.frequencyHz <= 0) return null;
  if (!isFiniteNumber(item.decibels)) return null;
  if (!isFiniteNumber(item.normalisedValue) || item.normalisedValue < 0 || item.normalisedValue > 1) return null;
  return {
    frequencyHz: item.frequencyHz,
    decibels: item.decibels,
    normalisedValue: item.normalisedValue,
  };
}

export function parseSpectrumSnapshot(value: unknown): SpectrumSnapshot | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (!isPositiveInteger(item.sequence)) return null;
  if (!isProfile(item.profile)) return null;
  if (!isFiniteNumber(item.sampleRate) || item.sampleRate <= 0) return null;
  if (!isPositiveInteger(item.fftSize)) return null;
  if (!isFiniteNumber(item.minFrequencyHz) || item.minFrequencyHz <= 0) return null;
  if (!isFiniteNumber(item.maxFrequencyHz) || item.maxFrequencyHz <= item.minFrequencyHz) return null;
  if (!isFiniteNumber(item.minDecibels) || !isFiniteNumber(item.maxDecibels) || item.minDecibels >= item.maxDecibels) return null;
  if (!Array.isArray(item.bins) || item.bins.length === 0 || item.bins.length > maxSpectrumBins) return null;

  const bins = item.bins.map(parseSpectrumBin);
  if (bins.some((bin) => bin === null)) return null;

  return {
    protocolVersion,
    sequence: item.sequence,
    profile: item.profile,
    sampleRate: item.sampleRate,
    fftSize: item.fftSize,
    minFrequencyHz: item.minFrequencyHz,
    maxFrequencyHz: item.maxFrequencyHz,
    minDecibels: item.minDecibels,
    maxDecibels: item.maxDecibels,
    bins: bins as SpectrumBin[],
  };
}
