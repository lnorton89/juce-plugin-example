import { projectConfig } from '../config/projectConfig';

export const protocolVersion = 1 as const;
export const uiReadyEvent = 'ui.ready' as const;
export const hostInfoEvent = 'host.info' as const;
export const bridgeErrorEvent = 'bridge.error' as const;
export const spectrumSnapshotEvent = 'spectrum.snapshot' as const;
export const sourceListEvent = 'source.list' as const;
export const sourceStateEvent = 'source.state' as const;
export const sourceSelectEvent = 'source.select' as const;
export const sourceStopEvent = 'source.stop' as const;
export const licenseStatusEvent = 'license.status' as const;
export const licenseActivateEvent = 'license.activate' as const;
export const licenseDeactivateEvent = 'license.deactivate' as const;
export const licenseValidateEvent = 'license.validate' as const;
export const maxSpectrumBins = 256 as const;

export interface UiReady { protocolVersion: typeof protocolVersion }
export interface HostInfo {
  protocolVersion: typeof protocolVersion;
  productName: typeof projectConfig.productName;
  companyName: typeof projectConfig.companyName;
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

export type SourceMode = 'InputDevice' | 'SystemOutput';

export type SourceState = 'stopped' | 'starting' | 'active' | 'silent' | 'error';

export interface SourceDescriptor {
  id: string;
  displayName: string;
  mode: SourceMode;
}

export interface SourceListPayload {
  protocolVersion: typeof protocolVersion;
  event: typeof sourceListEvent;
  inputDevices: SourceDescriptor[];
  systemOutputs: SourceDescriptor[];
}

export interface SourceStatePayload {
  protocolVersion: typeof protocolVersion;
  event: typeof sourceStateEvent;
  mode: SourceMode;
  state: SourceState;
  selectedSourceId: string;
  selectedSourceName: string;
  code: string;
  message: string;
}

export type LicensingStateEnum =
  | 'not_activated'
  | 'activating'
  | 'activated'
  | 'offline_grace'
  | 'revalidation_required'
  | 'revoked'
  | 'corrupt'
  | 'service_unavailable'
  | 'deactivating';

export interface LicenseStatusPayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseStatusEvent;
  state: LicensingStateEnum;
  activationId: string;
  lastVerifiedTime: string;
  offlineGraceRemainingDays: number;
  code?: string;
  message?: string;
}

export interface LicenseActivatePayload {
  protocolVersion: typeof protocolVersion;
  event: typeof licenseActivateEvent;
  licenseKey: string;
}

export type BridgeStatus =
  | { state: 'connecting'; spectrumSnapshot?: SpectrumSnapshot }
  | { state: 'ready'; hostInfo: HostInfo; spectrumSnapshot?: SpectrumSnapshot; sourceList?: SourceListPayload; sourceState?: SourceStatePayload }
  | { state: 'error'; code: string; message: string; spectrumSnapshot?: SpectrumSnapshot }; 

export function parseHostInfo(value: unknown): HostInfo | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  const bounded = (field: unknown) => typeof field === 'string' && field.length > 0 && field.length <= 128;
  return item.protocolVersion === protocolVersion && item.productName === projectConfig.productName
    && item.companyName === projectConfig.companyName && bounded(item.productVersion)
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

function parseSourceMode(value: unknown): SourceMode | null {
  return value === 'InputDevice' || value === 'SystemOutput' ? value : null;
}

function parseSourceStateEnum(value: unknown): SourceState | null {
  return value === 'stopped' || value === 'starting' || value === 'active' || value === 'silent' || value === 'error' ? value : null;
}

export function isLicenseState(value: unknown): value is LicensingStateEnum {
  return value === 'not_activated' || value === 'activating' || value === 'activated'
    || value === 'offline_grace' || value === 'revalidation_required'
    || value === 'revoked' || value === 'corrupt'
    || value === 'service_unavailable' || value === 'deactivating';
}

export function parseLicenseStatus(value: unknown): LicenseStatusPayload | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (item.event !== licenseStatusEvent) return null;
  if (!isLicenseState(item.state)) return null;
  if (typeof item.activationId !== 'string' || item.activationId.length > 64) return null;
  if (typeof item.lastVerifiedTime !== 'string' || item.lastVerifiedTime.length > 32) return null;
  if (typeof item.offlineGraceRemainingDays !== 'number') return null;
  if (item.code !== undefined && (typeof item.code !== 'string' || item.code.length > 64)) return null;
  if (item.message !== undefined && (typeof item.message !== 'string' || item.message.length > 256)) return null;
  return item as unknown as LicenseStatusPayload;
}

function parseSourceDescriptor(value: unknown): SourceDescriptor | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (typeof item.id !== 'string' || item.id.length === 0 || item.id.length > 256) return null;
  if (typeof item.displayName !== 'string' || item.displayName.length === 0 || item.displayName.length > 256) return null;
  const mode = parseSourceMode(item.mode);
  if (!mode) return null;
  return { id: item.id, displayName: item.displayName, mode };
}

export function parseSourceList(value: unknown): SourceListPayload | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (item.event !== sourceListEvent) return null;
  if (!Array.isArray(item.inputDevices) || !Array.isArray(item.systemOutputs)) return null;
  const inputDevices = item.inputDevices.map(parseSourceDescriptor);
  const systemOutputs = item.systemOutputs.map(parseSourceDescriptor);
  if (inputDevices.some(d => d === null) || systemOutputs.some(d => d === null)) return null;
  return { protocolVersion, event: sourceListEvent, inputDevices: inputDevices as SourceDescriptor[], systemOutputs: systemOutputs as SourceDescriptor[] };
}

export function parseSourceState(value: unknown): SourceStatePayload | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  if (item.protocolVersion !== protocolVersion) return null;
  if (item.event !== sourceStateEvent) return null;
  const mode = parseSourceMode(item.mode);
  const sourceStateVal = parseSourceStateEnum(item.state);
  if (!mode || !sourceStateVal) return null;
  if (typeof item.selectedSourceId !== 'string' || item.selectedSourceId.length > 256) return null;
  if (typeof item.selectedSourceName !== 'string' || item.selectedSourceName.length > 256) return null;
  if (typeof item.code !== 'string' || item.code.length > 64) return null;
  if (typeof item.message !== 'string' || item.message.length > 256) return null;
  return {
    protocolVersion,
    event: sourceStateEvent,
    mode,
    state: sourceStateVal,
    selectedSourceId: item.selectedSourceId,
    selectedSourceName: item.selectedSourceName,
    code: item.code,
    message: item.message,
  };
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
