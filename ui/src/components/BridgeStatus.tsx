import Typography from '@mui/material/Typography';
import type { BridgeStatus as BridgeState } from '../bridge/protocol';

export interface BridgeStatusProps { bridge: BridgeState }

export function BridgeStatus({ bridge }: BridgeStatusProps) {
  if (bridge.state === 'ready') return null;
  return <Typography role="status" aria-live="polite">{bridge.state === 'connecting' ? 'Connecting to audio engine…' : bridge.message}</Typography>;
}
