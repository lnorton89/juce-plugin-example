import { createContext, type PropsWithChildren, useContext, useEffect, useMemo, useState } from 'react';
import { bridgeErrorEvent, hostInfoEvent, parseBridgeError, parseHostInfo, parseSpectrumSnapshot, protocolVersion, spectrumSnapshotEvent, uiReadyEvent, type BridgeStatus } from './protocol';

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
    transport.emitEvent(uiReadyEvent, { protocolVersion });
    return () => {
      transport.removeEventListener(hostToken);
      transport.removeEventListener(errorToken);
      transport.removeEventListener(snapshotToken);
    };
  }, [initialStatus, transport]);

  return <BridgeContext.Provider value={status}>{children}</BridgeContext.Provider>;
}

export function useBridgeStatus() {
  return useContext(BridgeContext);
}
