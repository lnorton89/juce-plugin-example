import { createContext, type PropsWithChildren, useCallback, useContext, useEffect, useMemo, useState } from 'react';
import { bridgeErrorEvent, hostInfoEvent, parseBridgeError, parseHostInfo, parseSourceList, parseSourceState, parseSpectrumSnapshot, protocolVersion, sourceListEvent, sourceSelectEvent, sourceStateEvent, sourceStopEvent, spectrumSnapshotEvent, uiReadyEvent, type BridgeStatus, type SourceListPayload, type SourceStatePayload } from './protocol';

export interface BridgeBackend {
  addEventListener(eventId: string, listener: (payload: unknown) => void): unknown;
  removeEventListener(token: unknown): void;
  emitEvent(eventId: string, payload: unknown): void;
}

declare global {
  interface Window { __JUCE__?: { backend?: BridgeBackend } }
}

const BridgeContext = createContext<BridgeStatus>({ state: 'connecting' });

export interface BridgeProviderProps extends PropsWithChildren {
  initialStatus?: BridgeStatus;
  backend?: BridgeBackend;
}

export function BridgeProvider({ children, initialStatus, backend }: BridgeProviderProps) {
  const [status, setStatus] = useState<BridgeStatus>(initialStatus ?? { state: 'connecting' });
  const transport = useMemo(() => backend ?? window.__JUCE__?.backend, [backend]);

  useEffect(() => {
    if (initialStatus || !transport) return;
    const hostToken = transport.addEventListener(hostInfoEvent, (payload) => {
      const hostInfo = parseHostInfo(payload);
      setStatus(hostInfo ? { state: 'ready', hostInfo } : { state: 'error', code: 'malformed_host_info', message: 'Native host information was invalid.' });
    });
    const errorToken = transport.addEventListener(bridgeErrorEvent, (payload) => {
      const error = parseBridgeError(payload);
      setStatus(error ? { state: 'error', code: error.code, message: error.message } : { state: 'error', code: 'malformed_error', message: 'Native bridge error was invalid.' });
    });
    const snapshotToken = transport.addEventListener(spectrumSnapshotEvent, (payload) => {
      const snapshot = parseSpectrumSnapshot(payload);
      if (snapshot) setStatus((current) => ({ ...current, spectrumSnapshot: snapshot }));
    });
    const sourceListToken = transport.addEventListener(sourceListEvent, (payload) => {
      const sourceList = parseSourceList(payload);
      if (sourceList) setStatus((current) => ({ ...current, sourceList }));
    });
    const sourceStateToken = transport.addEventListener(sourceStateEvent, (payload) => {
      const sourceState = parseSourceState(payload);
      if (sourceState) setStatus((current) => ({ ...current, sourceState }));
    });
    transport.emitEvent(uiReadyEvent, { protocolVersion });
    return () => {
      transport.removeEventListener(hostToken);
      transport.removeEventListener(errorToken);
      transport.removeEventListener(snapshotToken);
      transport.removeEventListener(sourceListToken);
      transport.removeEventListener(sourceStateToken);
    };
  }, [initialStatus, transport]);

  return <BridgeContext.Provider value={status}>{children}</BridgeContext.Provider>;
}

export function useBridgeStatus(): BridgeStatus {
  return useContext(BridgeContext);
}

export function useBridgeBackend(): BridgeBackend | undefined {
  const transport = useMemo(() => { throw new Error('not implemented'); }, []);
  // Bridge request helpers are accessed through useBridgeRequest instead
  return undefined;
}

export interface BridgeRequestHelpers {
  sendSourceSelect: (mode: string, sourceId: string) => void;
  sendSourceStop: () => void;
}

export function useBridgeRequest(): BridgeRequestHelpers | null {
  const transport = useMemo(() => {
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    return (typeof window !== 'undefined') ? (window.__JUCE__?.backend ?? null) : null;
  }, []);

  const sendSourceSelect = useCallback((mode: string, sourceId: string) => {
    if (!transport) return;
    transport.emitEvent(sourceSelectEvent, { protocolVersion, mode, sourceId });
  }, [transport]);

  const sendSourceStop = useCallback(() => {
    if (!transport) return;
    transport.emitEvent(sourceStopEvent, { protocolVersion });
  }, [transport]);

  if (!transport) return null;
  return { sendSourceSelect, sendSourceStop };
}
