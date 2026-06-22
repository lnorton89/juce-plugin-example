import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import type { BridgeStatus } from '../bridge/protocol';

export interface StatusFooterProps { bridge: BridgeStatus }

export function StatusFooter({ bridge }: StatusFooterProps) {
  const text = bridge.state === 'connecting' ? 'Connecting to audio engine…'
    : bridge.state === 'ready' ? `${bridge.hostInfo.uiSource === 'embedded' ? 'Embedded UI' : 'Vite development'} · Bridge ready`
      : bridge.message;
  return (
    <Box component="footer" sx={{ minWidth: 0, display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 4, px: { xs: 3, sm: 5 }, borderTop: 1, borderColor: 'border', bgcolor: 'background.paper' }}>
      <Typography role="status" aria-live="polite" variant="body2" color="text.secondary" noWrap>{text}</Typography>
      <Typography color="text.secondary" noWrap sx={{ '@media (max-width:719px)': { display: 'none' } }}>Signal Foundry Audio</Typography>
    </Box>
  );
}
