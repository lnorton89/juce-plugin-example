import CheckCircleOutlineIcon from '@mui/icons-material/CheckCircleOutlineOutlined';
import ErrorOutlineIcon from '@mui/icons-material/ErrorOutlineOutlined';
import HourglassEmptyIcon from '@mui/icons-material/HourglassEmpty';
import WarningAmberIcon from '@mui/icons-material/WarningAmber';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import type { BridgeStatus as BridgeState } from '../bridge/protocol';

export interface BridgeStatusProps { bridge: BridgeState; onRetry?: () => void }
export type PresentationKind = 'connecting' | 'ready' | 'development-server-unavailable' | 'protocol-mismatch' | 'runtime-error';

export function presentationKind(bridge: BridgeState): PresentationKind {
  if (bridge.state !== 'error') return bridge.state;
  if (bridge.code === 'development_server_unavailable' || bridge.code === 'dev_server_unavailable') return 'development-server-unavailable';
  if (bridge.code === 'protocol_mismatch') return 'protocol-mismatch';
  return 'runtime-error';
}

export function boundedDiagnostics(bridge: Extract<BridgeState, { state: 'error' }>) {
  return `${bridge.code.slice(0, 64)}: ${bridge.message.slice(0, 256)}`;
}

const copyByKind = {
  'development-server-unavailable': 'Development server unavailable. Start the UI dev task or switch to embedded assets.',
  'protocol-mismatch': 'Interface version mismatch. Rebuild the native app and embedded UI from the same checkout.',
  'runtime-error': 'The interface could not start. Check the WebView2 runtime and reopen LumaScope.',
} as const;

export function BridgeStatus({ bridge, onRetry }: BridgeStatusProps) {
  const kind = presentationKind(bridge);
  if (kind === 'ready') return null;
  if (kind === 'connecting') {
    return (
      <Box role="status" aria-live="polite" sx={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 2 }}>
        <HourglassEmptyIcon aria-hidden="true" fontSize="small" sx={{ color: 'primary.main' }} />
        <Typography>Connecting to audio engine…</Typography>
      </Box>
    );
  }

  const retryable = kind === 'development-server-unavailable';
  const Icon = retryable ? WarningAmberIcon : ErrorOutlineIcon;
  return (
    <Box sx={{ display: 'grid', justifyItems: 'center', gap: 3, minWidth: 0 }}>
      <Icon aria-hidden="true" sx={{ color: retryable ? 'status.warning' : 'status.destructive' }} />
      <Typography component="h2" variant="h2">Interface connection issue</Typography>
      <Typography role="status" aria-live="polite" sx={{ maxWidth: '70ch' }}>{copyByKind[kind]}</Typography>
      {bridge.state === 'error' && (
        <Typography aria-label="Diagnostic details" title={boundedDiagnostics(bridge)} sx={{ maxWidth: '100%', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', color: 'text.secondary', fontFamily: '"Cascadia Mono", Consolas, monospace', fontSize: 12 }}>
          {boundedDiagnostics(bridge)}
        </Typography>
      )}
      {retryable && <Button variant="outlined" onClick={onRetry}>Retry connection</Button>}
    </Box>
  );
}

export function ReadyStatusIcon() {
  return <CheckCircleOutlineIcon aria-hidden="true" fontSize="small" sx={{ color: 'status.ready' }} />;
}
