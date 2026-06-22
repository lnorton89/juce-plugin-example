import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import App from './App';
import { BridgeProvider } from './bridge/BridgeProvider';

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BridgeProvider>
      <App />
    </BridgeProvider>
  </StrictMode>,
);

