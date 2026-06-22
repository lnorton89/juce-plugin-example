import CssBaseline from '@mui/material/CssBaseline';
import { ThemeProvider } from '@mui/material/styles';
import { AppShell } from './app/AppShell';
import { lumaScopeTheme } from './app/theme';

export default function App() {
  return (
    <ThemeProvider theme={lumaScopeTheme}>
      <CssBaseline />
      <AppShell />
    </ThemeProvider>
  );
}
