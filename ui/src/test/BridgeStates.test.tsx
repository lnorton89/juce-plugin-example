import { fireEvent, render, screen } from '@testing-library/react';
import { beforeEach, describe, expect, test, vi } from 'vitest';
import App from '../App';
import { BridgeProvider } from '../bridge/BridgeProvider';
import type { BridgeStatus } from '../bridge/protocol';

const ready = (hostMode: 'VST3' | 'Standalone', uiSource: 'embedded' | 'vite'): BridgeStatus => ({
  state: 'ready',
  hostInfo: { protocolVersion: 1, productName: 'LumaScope', companyName: 'Signal Foundry Audio', productVersion: '0.1.0', hostMode, uiSource, buildMarker: '0.1.0-Debug' },
});
const error = (code: string, message = 'diagnostic'): BridgeStatus => ({ state: 'error', code, message });
const renderState = (status: BridgeStatus) => render(<BridgeProvider initialStatus={status}><App /></BridgeProvider>);

beforeEach(() => {
  Object.assign(navigator, { clipboard: { writeText: vi.fn().mockResolvedValue(undefined) } });
});

describe('approved bridge state copy', () => {
  test('connecting', () => {
    renderState({ state: 'connecting' });
    expect(screen.getAllByText('Connecting to audio engine…')).toHaveLength(2);
  });

  test.each([
    ['Standalone', 'embedded', 'Embedded UI · Bridge ready', true],
    ['VST3', 'vite', 'Vite development · Bridge ready', false],
  ] as const)('ready %s %s', (hostMode, uiSource, copy, isStandalone) => {
    renderState(ready(hostMode, uiSource));
    expect(screen.getByLabelText('Product mode')).toHaveTextContent(hostMode);
    // Standalone mode has two status elements (source strip + footer);
    // VST3 has only the footer status.
    if (isStandalone) {
      expect(screen.getAllByRole('status')[1]).toHaveTextContent(copy);
    } else {
      expect(screen.getByRole('status')).toHaveTextContent(copy);
    }
  });

  test('development server unavailable is the only retryable error', () => {
    renderState(error('development_server_unavailable'));
    expect(screen.getByRole('status')).toHaveTextContent('Development server unavailable. Start the UI dev task or switch to embedded assets.');
    expect(screen.getByRole('button', { name: 'Retry connection' })).toBeInTheDocument();
  });

  test.each([
    ['protocol_mismatch', 'Interface version mismatch. Rebuild the native app and embedded UI from the same checkout.'],
    ['runtime_error', 'The interface could not start. Check the WebView2 runtime and reopen LumaScope.'],
  ])('%s error', (code, copy) => {
    renderState(error(code));
    expect(screen.getByRole('status')).toHaveTextContent(copy);
    expect(screen.queryByRole('button', { name: 'Retry connection' })).not.toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Copy diagnostics' })).toBeInTheDocument();
  });
});

test('diagnostics copy is bounded and announced', async () => {
  renderState(error('runtime_error', '<img src=x onerror=alert(1)>' + 'x'.repeat(400)));
  expect(screen.getByLabelText('Diagnostic details')).toHaveTextContent('<img src=x onerror=alert(1)>');
  expect(document.querySelector('img')).toBeNull();
  fireEvent.click(screen.getByRole('button', { name: 'Copy diagnostics' }));
  expect(await screen.findByText('Diagnostics copied')).toBeInTheDocument();
  expect(vi.mocked(navigator.clipboard.writeText).mock.calls[0][0].length).toBeLessThanOrEqual(322);
});
