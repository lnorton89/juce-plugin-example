import { render, screen } from '@testing-library/react';
import { expect, test } from 'vitest';
import App from '../App';
import { BridgeProvider } from '../bridge/BridgeProvider';

test('renders the semantic three-row shell while connecting', () => {
  const { container } = render(<BridgeProvider><App /></BridgeProvider>);
  expect(screen.getByRole('heading', { name: 'LumaScope' })).toBeInTheDocument();
  expect(screen.getByRole('region', { name: 'Spectrum display' })).toBeInTheDocument();
  expect(screen.getAllByRole('status')[0]).toHaveTextContent('Connecting to audio engine');
  expect(container.querySelector('[data-phase="renderer-mount"]')).toBeInTheDocument();
});

test('renders honest ready copy with source controls but no future settings/license/Hertz/dB values', () => {
  render(
    <BridgeProvider initialStatus={{ state: 'ready', hostInfo: { protocolVersion: 1, productName: 'LumaScope', companyName: 'Signal Foundry Audio', productVersion: '0.1.0', hostMode: 'Standalone', uiSource: 'embedded', buildMarker: '0.1.0-Debug' } }}>
      <App />
    </BridgeProvider>,
  );
  expect(screen.getByRole('heading', { name: 'Analyzer surface ready' })).toBeInTheDocument();
  expect(screen.getByText('Live spectrum data will appear here when analysis is enabled.')).toBeInTheDocument();
  // Source controls are present in Standalone mode (Phase 3)
  // (MUI renders both <label> and <legend>, so use getAllByText)
  expect(screen.getAllByText('Source mode').length).toBeGreaterThanOrEqual(1);
  expect(screen.getAllByText('Select source').length).toBeGreaterThanOrEqual(1);
  // But settings, license, Hz, and dB controls have not been introduced yet
  expect(screen.queryByText(/Hz|dB|settings|license/i)).not.toBeInTheDocument();
});
