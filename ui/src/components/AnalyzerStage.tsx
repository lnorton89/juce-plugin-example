import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import type { BridgeStatus as BridgeState } from '../bridge/protocol';
import { BridgeStatus } from './BridgeStatus';
import '../styles/spectral-motif.css';

export interface AnalyzerStageProps { bridge: BridgeState }

export function AnalyzerStage({ bridge }: AnalyzerStageProps) {
  return (
    <Box
      component="section"
      aria-label="Spectrum display"
      sx={{
        position: 'relative',
        isolation: 'isolate',
        width: '100%',
        height: '100%',
        minWidth: 0,
        minHeight: 0,
        overflow: 'hidden',
        display: 'grid',
        placeItems: 'center',
        border: 1,
        borderColor: 'border',
        borderRadius: 2.5,
        bgcolor: 'background.paper',
        boxShadow: 1,
        px: { xs: 6, sm: 12 },
        py: 6,
      }}
    >
      <div className="spectral-motif" aria-hidden="true"><span /></div>
      <Box id="spectrum-renderer-mount" data-phase="renderer-mount" sx={{ position: 'absolute', inset: 0, pointerEvents: 'none' }} />
      <Box sx={{ position: 'relative', zIndex: 1, width: 'min(100%, 560px)', textAlign: 'center' }}>
        {bridge.state === 'ready' ? (
          <>
            <Typography component="h2" variant="h2">Analyzer surface ready</Typography>
            <Typography color="text.secondary" sx={{ mt: 2 }}>
              Live spectrum data will appear here when analysis is enabled.
            </Typography>
          </>
        ) : null}
        <BridgeStatus bridge={bridge} />
      </Box>
    </Box>
  );
}
