import { createContext, type PropsWithChildren, useContext, useMemo } from 'react';
import type { BridgeStatus } from './protocol';

const BridgeContext = createContext<BridgeStatus>({ state: 'connecting' });

export interface BridgeProviderProps extends PropsWithChildren {
  initialStatus?: BridgeStatus;
}

export function BridgeProvider({ children, initialStatus }: BridgeProviderProps) {
  const status = useMemo<BridgeStatus>(() => initialStatus ?? { state: 'connecting' }, [initialStatus]);
  return <BridgeContext.Provider value={status}>{children}</BridgeContext.Provider>;
}

export function useBridgeStatus() {
  return useContext(BridgeContext);
}

