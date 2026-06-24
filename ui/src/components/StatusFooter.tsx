import ContentCopyIcon from '@mui/icons-material/ContentCopy';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import { useState } from 'react';
import type { LicenseStatusPayload } from '../bridge/protocol';
import type { BridgeStatus as BridgeState } from '../bridge/protocol';
import { ActivationStatus } from './ActivationStatus';
import { boundedDiagnostics, presentationKind, ReadyStatusIcon } from './BridgeStatus';

export interface StatusFooterProps {
  bridge: BridgeState;
  licensing?: LicenseStatusPayload | null;
  onLicenseClick: () => void;
}

export function StatusFooter({ bridge, licensing, onLicenseClick }: StatusFooterProps) {
  const [copyResult, setCopyResult] = useState('');
  const kind = presentationKind(bridge);
  const text = bridge.state === 'connecting' ? 'Connecting to audio engine\u2026'
    : bridge.state === 'ready' ? `${bridge.hostInfo.uiSource === 'embedded' ? 'Embedded UI' : 'Vite development'} \u00B7 Bridge ready`
      : kind === 'development-server-unavailable' ? 'Vite development \u00B7 Connection issue'
        : 'Embedded UI \u00B7 Connection issue';

  async function copyDiagnostics() {
    if (bridge.state !== 'error') return;
    try {
      await navigator.clipboard.writeText(boundedDiagnostics(bridge));
      setCopyResult('Diagnostics copied');
    } catch {
      setCopyResult('Copy unavailable');
    }
  }

  return (
    <Box component="footer" sx={{ minWidth: 0, display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 4, px: { xs: 3, sm: 5 }, borderTop: 1, borderColor: 'border', bgcolor: 'background.paper' }}>
      <Box sx={{ minWidth: 0, display: 'flex', alignItems: 'center', gap: 2, overflow: 'hidden' }}>
        {bridge.state === 'ready' && <ReadyStatusIcon />}
        <Typography role={bridge.state === 'ready' ? 'status' : undefined} aria-live={bridge.state === 'ready' ? 'polite' : undefined} variant="body2" color="text.secondary" noWrap>{text}</Typography>
        {bridge.state === 'error' && <Button size="small" startIcon={<ContentCopyIcon aria-hidden="true" />} onClick={copyDiagnostics} sx={{ minHeight: 32, whiteSpace: 'nowrap' }}>Copy diagnostics</Button>}
        {copyResult && <Typography role="status" aria-live="polite" variant="body2" noWrap>{copyResult}</Typography>}
      </Box>
      {licensing && (
        <ActivationStatus
          state={licensing.state}
          onClick={onLicenseClick}
          offlineGraceRemainingDays={licensing.offlineGraceRemainingDays}
        />
      )}
      <Typography color="text.secondary" noWrap sx={{ '@media (max-width:719px)': { display: 'none' } }}>Signal Foundry Audio</Typography>
    </Box>
  );
}
