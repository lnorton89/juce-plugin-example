export const protocolVersion = 1 as const;

export type BridgeStatus =
  | { state: 'connecting' }
  | { state: 'ready'; hostMode: 'VST3' | 'Standalone'; uiSource: 'embedded' | 'vite' }
  | { state: 'error'; code: string; message: string };

