import { render, screen } from '@testing-library/react';
import { beforeEach, expect, test, vi } from 'vitest';
import spectrumSnapshot from '../../../tests/fixtures/bridge/spectrum-snapshot-v1.json';
import type { BridgeStatus } from '../bridge/protocol';
import { AnalyzerStage } from './AnalyzerStage';

beforeEach(() => {
  Object.defineProperty(HTMLCanvasElement.prototype, 'clientWidth', { configurable: true, value: 640 });
  Object.defineProperty(HTMLCanvasElement.prototype, 'clientHeight', { configurable: true, value: 280 });
  HTMLCanvasElement.prototype.getContext = vi.fn(() => ({
    clearRect: vi.fn(),
    beginPath: vi.fn(),
    moveTo: vi.fn(),
    lineTo: vi.fn(),
    bezierCurveTo: vi.fn(),
    closePath: vi.fn(),
    fill: vi.fn(),
    stroke: vi.fn(),
    save: vi.fn(),
    restore: vi.fn(),
    scale: vi.fn(),
    setTransform: vi.fn(),
  } as unknown as CanvasRenderingContext2D));
});

const readyBridge: BridgeStatus = {
  state: 'ready',
  hostInfo: {
    protocolVersion: 1,
    productName: 'LumaScope',
    companyName: 'Signal Foundry Audio',
    productVersion: '0.1.0',
    hostMode: 'VST3',
    uiSource: 'embedded',
    buildMarker: '0.1.0-Debug',
  },
  spectrumSnapshot,
};

test('mounts the spectrum canvas inside the existing renderer mount', () => {
  const { container } = render(<AnalyzerStage bridge={readyBridge} />);
  const mount = container.querySelector('#spectrum-renderer-mount');

  expect(mount).toBeInTheDocument();
  expect(mount).toContainElement(screen.getByTestId('spectrum-canvas'));
});
