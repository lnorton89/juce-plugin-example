import Box from '@mui/material/Box';
import CssBaseline from '@mui/material/CssBaseline';
import Typography from '@mui/material/Typography';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import { useBridgeStatus } from './bridge/BridgeProvider';

const theme = createTheme({
  cssVariables: true,
  palette: {
    mode: 'dark',
    background: { default: '#0B0F12', paper: '#121A20' },
    text: { primary: '#F2F7F8', secondary: '#91A4AE' },
    primary: { main: '#53D8FB' },
    success: { main: '#B7F36B' },
  },
  typography: { fontFamily: '"Segoe UI Variable", "Segoe UI", system-ui, sans-serif' },
});

export default function App() {
  const bridge = useBridgeStatus();
  const status = bridge.state === 'connecting' ? 'Connecting to audio engine…'
    : bridge.state === 'ready' ? `${bridge.hostInfo.uiSource === 'embedded' ? 'Embedded UI' : 'Vite development'} · Bridge ready`
      : bridge.message;
  const mode = bridge.state === 'ready' ? bridge.hostInfo.hostMode : '';

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <Box component="main" sx={{ minHeight: '100vh', display: 'grid', placeItems: 'center', p: 2 }}>
        <Box component="section" aria-label="Spectrum display" sx={{ width: '100%', maxWidth: 928, minHeight: 328, bgcolor: 'background.paper', border: 1, borderColor: '#24323A', borderRadius: '10px', p: 4 }}>
          <Typography component="h1" variant="h4">LumaScope</Typography>
          <Typography color="text.secondary">Spectrum analyzer</Typography>
          {mode && <Typography aria-label="Product mode" sx={{ mt: 2 }}>{mode}</Typography>}
          <Typography role="status" aria-live="polite" sx={{ mt: 4 }}>{status}</Typography>
        </Box>
      </Box>
    </ThemeProvider>
  );
}
