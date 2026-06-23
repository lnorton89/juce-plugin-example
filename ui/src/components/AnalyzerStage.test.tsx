import { render, screen } from '@testing-library/react';
import { expect, test } from 'vitest';
import spectrumSnapshot from '../../../tests/fixtures/bridge/spectrum-snapshot-v1.json';
import type { BridgeStatus } from '../bridge/protocol';
import { AnalyzerStage } from './AnalyzerStage';

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
