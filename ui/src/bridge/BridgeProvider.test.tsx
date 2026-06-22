import { render, screen } from '@testing-library/react';
import { expect, test } from 'vitest';
import App from '../App';
import { BridgeProvider, type BridgeBackend } from './BridgeProvider';
import hostInfo from '../../../tests/fixtures/bridge/host-info-v1.json';
import malformed from '../../../tests/fixtures/bridge/malformed.json';
import uiReady from '../../../tests/fixtures/bridge/ui-ready-v1.json';
import unsupported from '../../../tests/fixtures/bridge/unsupported-version.json';

class MockBackend implements BridgeBackend {
  listeners = new Map<string, (payload: unknown) => void>();
  emitted: Array<[string, unknown]> = [];
  addEventListener(id: string, listener: (payload: unknown) => void) { this.listeners.set(id, listener); return id; }
  removeEventListener(token: unknown) { this.listeners.delete(String(token)); }
  emitEvent(id: string, payload: unknown) { this.emitted.push([id, payload]); }
  reply(id: string, payload: unknown) { this.listeners.get(id)?.(payload); }
}

test('valid fixture completes the protocol-v1 handshake', async () => {
  const backend = new MockBackend();
  render(<BridgeProvider backend={backend}><App /></BridgeProvider>);
  expect(backend.emitted).toContainEqual(['ui.ready', uiReady]);
  backend.reply('host.info', hostInfo);
  expect(await screen.findByText('Standalone')).toBeInTheDocument();
  expect(screen.getByRole('status')).toHaveTextContent('Embedded UI · Bridge ready');
});

test('unsupported and malformed fixtures never reach ready state', async () => {
  const backend = new MockBackend();
  render(<BridgeProvider backend={backend}><App /></BridgeProvider>);
  backend.reply('host.info', unsupported);
  expect(await screen.findByRole('status')).toHaveTextContent('Native host information was invalid');
  backend.reply('host.info', malformed);
  expect(screen.queryByText('Bridge ready')).not.toBeInTheDocument();
});
