import { useCallback } from 'react';
import Box from '@mui/material/Box';
import { useBridgeRequest, useBridgeStatus } from '../bridge/BridgeProvider';
import { AnalyzerStage } from '../components/AnalyzerStage';
import { BrandHeader } from '../components/BrandHeader';
import { StandaloneSourceStrip } from '../components/StandaloneSourceStrip';
import { StatusFooter } from '../components/StatusFooter';

export function AppShell() {
  const bridge = useBridgeStatus();
  const request = useBridgeRequest();
  const mode = bridge.state === 'ready' ? bridge.hostInfo.hostMode : undefined;
  const retryConnection = useCallback(() => window.location.reload(), []);

  const handleSourceSelect = useCallback((mode: string, sourceId: string) => {
    request?.sendSourceSelect(mode, sourceId);
  }, [request]);

  const handleSourceStop = useCallback(() => {
    request?.sendSourceStop();
  }, [request]);

  return (
    <Box
      sx={{
        width: '100vw',
        height: '100vh',
        minWidth: 0,
        display: 'grid',
        gridTemplateRows: '52px auto minmax(0, 1fr) 32px',
        bgcolor: 'background.default',
        color: 'text.primary',
        overflow: 'hidden',
      }}
    >
      <BrandHeader mode={mode} />
      <StandaloneSourceStrip
        bridge={bridge}
        onSourceSelect={handleSourceSelect}
        onSourceStop={handleSourceStop}
      />
      <Box
        component="main"
        sx={{ minWidth: 0, minHeight: 0, p: 4, '@media (max-width:639px)': { p: 3 } }}
      >
        <AnalyzerStage bridge={bridge} onRetry={retryConnection} />
      </Box>
      <StatusFooter bridge={bridge} />
    </Box>
  );
}
