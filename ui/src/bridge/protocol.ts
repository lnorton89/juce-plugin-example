export const protocolVersion = 1 as const;
export const uiReadyEvent = 'ui.ready' as const;
export const hostInfoEvent = 'host.info' as const;
export const bridgeErrorEvent = 'bridge.error' as const;

export interface UiReady { protocolVersion: typeof protocolVersion }
export interface HostInfo {
  protocolVersion: typeof protocolVersion;
  productName: 'LumaScope';
  companyName: 'Signal Foundry Audio';
  productVersion: string;
  hostMode: 'VST3' | 'Standalone';
  uiSource: 'embedded' | 'vite';
}
export interface BridgeError { code: string; message: string; protocolVersion: number }

export type BridgeStatus =
  | { state: 'connecting' }
  | { state: 'ready'; hostInfo: HostInfo }
  | { state: 'error'; code: string; message: string };

export function parseHostInfo(value: unknown): HostInfo | null {
  if (!value || typeof value !== 'object') return null;
  const item = value as Record<string, unknown>;
  const bounded = (field: unknown) => typeof field === 'string' && field.length > 0 && field.length <= 128;
  return item.protocolVersion === protocolVersion && item.productName === 'LumaScope'
    && item.companyName === 'Signal Foundry Audio' && bounded(item.productVersion)
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
