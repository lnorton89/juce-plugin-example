import { createTheme } from '@mui/material/styles';

declare module '@mui/material/styles' {
  interface Palette {
    raised: string;
    border: string;
    status: { ready: string; warning: string; destructive: string };
  }
  interface PaletteOptions {
    raised?: string;
    border?: string;
    status?: { ready: string; warning: string; destructive: string };
  }
}

export const lumaScopeTheme = createTheme({
  cssVariables: true,
  spacing: 4,
  palette: {
    mode: 'dark',
    background: { default: '#0B0F12', paper: '#121A20' },
    raised: '#172129',
    border: '#24323A',
    text: { primary: '#F2F7F8', secondary: '#91A4AE' },
    primary: { main: '#53D8FB' },
    success: { main: '#B7F36B' },
    warning: { main: '#FFC857' },
    error: { main: '#FF6B78' },
    status: { ready: '#B7F36B', warning: '#FFC857', destructive: '#FF6B78' },
  },
  shape: { borderRadius: 10 },
  shadows: [
    'none',
    '0 16px 48px rgba(0,0,0,.32)',
    ...Array(23).fill('none'),
  ] as ReturnType<typeof createTheme>['shadows'],
  typography: {
    fontFamily: '"Segoe UI Variable", "Segoe UI", system-ui, sans-serif',
    fontSize: 14,
    body1: { fontSize: 14, lineHeight: 1.5 },
    body2: { fontSize: 14, lineHeight: 1.5 },
    subtitle2: { fontSize: 12, fontWeight: 600, lineHeight: 1.35, letterSpacing: '0.06em' },
    h2: { fontSize: 20, fontWeight: 600, lineHeight: 1.25, letterSpacing: '-0.01em' },
    h1: { fontSize: 28, fontWeight: 650, lineHeight: 1.15, letterSpacing: '-0.025em' },
  },
  components: {
    MuiCssBaseline: {
      styleOverrides: {
        html: { width: '100%', height: '100%', backgroundColor: '#0B0F12' },
        body: { width: '100%', height: '100%', margin: 0, overflow: 'hidden' },
        '#root': { width: '100%', height: '100%' },
        'button:focus-visible': { outline: '2px solid #53D8FB', outlineOffset: 2 },
        '@media (forced-colors: active)': {
          '*': { forcedColorAdjust: 'auto' },
          'button:focus-visible': { outlineColor: 'Highlight' },
        },
      },
    },
    MuiButton: {
      styleOverrides: {
        root: { minWidth: 32, minHeight: 36, textTransform: 'none', borderRadius: 6 },
      },
    },
  },
});
