import Box from '@mui/material/Box';
import { useBridgeStatus } from '../bridge/BridgeProvider';
import { AnalyzerStage } from '../components/AnalyzerStage';
import { BrandHeader } from '../components/BrandHeader';
import { StatusFooter } from '../components/StatusFooter';

export function AppShell() {
  const bridge = useBridgeStatus();
  const mode = bridge.state === 'ready' ? bridge.hostInfo.hostMode : undefined;

  return (
    <Box
      sx={{
        width: '100vw',
        height: '100vh',
        minWidth: 0,
        display: 'grid',
        gridTemplateRows: '52px minmax(0, 1fr) 32px',
        bgcolor: 'background.default',
        color: 'text.primary',
        overflow: 'hidden',
      }}
    >
      <BrandHeader mode={mode} />
      <Box
        component="main"
        sx={{ minWidth: 0, minHeight: 0, p: 4, '@media (max-width:639px)': { p: 3 } }}
      >
        <AnalyzerStage bridge={bridge} />
      </Box>
      <StatusFooter bridge={bridge} />
    </Box>
  );
}
