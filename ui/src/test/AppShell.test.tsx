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

test('renders honest ready copy without future controls or fabricated values', () => {
  render(
    <BridgeProvider initialStatus={{ state: 'ready', hostInfo: { protocolVersion: 1, productName: 'LumaScope', companyName: 'Signal Foundry Audio', productVersion: '0.1.0', hostMode: 'Standalone', uiSource: 'embedded' } }}>
      <App />
    </BridgeProvider>,
  );
  expect(screen.getByRole('heading', { name: 'Analyzer surface ready' })).toBeInTheDocument();
  expect(screen.getByText('Live spectrum data will appear here when analysis is enabled.')).toBeInTheDocument();
  expect(screen.queryByText(/Hz|dB|settings|license|source/i)).not.toBeInTheDocument();
});
