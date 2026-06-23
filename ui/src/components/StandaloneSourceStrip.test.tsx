import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { beforeEach, expect, test, vi } from 'vitest';
import type { BridgeStatus, SourceDescriptor, SourceListPayload, SourceStatePayload } from '../bridge/protocol';
import { StandaloneSourceStrip } from './StandaloneSourceStrip';

const emptySourceList: SourceListPayload = {
  protocolVersion: 1,
  event: 'source.list',
  inputDevices: [],
  systemOutputs: [],
};

const micList: SourceDescriptor[] = [
  { id: 'juce-input-mic-1', displayName: 'Microphone (Realtek Audio)', mode: 'InputDevice' },
];

const speakerList: SourceDescriptor[] = [
  { id: 'wasapi-speaker-1', displayName: 'Speakers (Realtek Audio)', mode: 'SystemOutput' },
];

const fullSourceList: SourceListPayload = {
  protocolVersion: 1,
  event: 'source.list',
  inputDevices: micList,
  systemOutputs: speakerList,
};

const readyBridgeVST3: BridgeStatus = {
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
};

const readyBridgeStandalone: BridgeStatus = {
  state: 'ready',
  hostInfo: {
    protocolVersion: 1,
    productName: 'LumaScope',
    companyName: 'Signal Foundry Audio',
    productVersion: '0.1.0',
    hostMode: 'Standalone',
    uiSource: 'embedded',
    buildMarker: '0.1.0-Debug',
  },
};

const defaultSelectHandler = vi.fn();
const defaultStopHandler = vi.fn();

beforeEach(() => {
  vi.clearAllMocks();
});

test('renders nothing when bridge is not ready', () => {
  const { container } = render(
    <StandaloneSourceStrip
      bridge={{ state: 'connecting' }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(container.innerHTML).toBe('');
});

test('renders nothing when hostMode is VST3 for UI-04', () => {
  const { container } = render(
    <StandaloneSourceStrip
      bridge={readyBridgeVST3}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(container.innerHTML).toBe('');
});

test('renders source strip when hostMode is Standalone', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'stopped', selectedSourceId: '', selectedSourceName: '', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByLabelText('Source mode')).toBeInTheDocument();
  expect(screen.getByRole('combobox', { name: /source mode/i })).toBeInTheDocument();
});

test('shows Choose source status when stopped', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'stopped', selectedSourceId: '', selectedSourceName: '', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByText('Choose source')).toBeInTheDocument();
});

test('shows Active status when source is active', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'active', selectedSourceId: 'juce-input-mic-1', selectedSourceName: 'Microphone (Realtek Audio)', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByText(/Active/)).toBeInTheDocument();
  expect(screen.getByText('Microphone (Realtek Audio)')).toBeInTheDocument();
});

test('shows No signal status when silent', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'silent', selectedSourceId: 'juce-input-mic-1', selectedSourceName: 'Microphone (Realtek Audio)', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByText(/No signal detected/i)).toBeInTheDocument();
});

test('shows error status with message when state is error', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'error', selectedSourceId: 'juce-input-mic-1', selectedSourceName: 'Microphone', code: 'source_lost', message: 'The selected device was disconnected.' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByText(/source_lost/)).toBeInTheDocument();
});

test('shows starting status when state is starting', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'starting', selectedSourceId: 'juce-input-mic-1', selectedSourceName: 'Microphone (Realtek Audio)', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByText(/Starting/)).toBeInTheDocument();
});

test('calls onSourceSelect when choosing an input source', async () => {
  const user = userEvent.setup();
  const onSelect = vi.fn();

  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'stopped', selectedSourceId: '', selectedSourceName: '', code: '', message: '' } }}
      onSourceSelect={onSelect}
      onSourceStop={defaultStopHandler}
    />,
  );

  // Open the source select dropdown and pick the microphone
  const sourceSelect = screen.getByRole('combobox', { name: /select source/i });
  await user.click(sourceSelect);
  const micOption = await screen.findByRole('option', { name: /microphone/i });
  await user.click(micOption);

  expect(onSelect).toHaveBeenCalledWith('InputDevice', 'juce-input-mic-1');
});

test('calls onSourceStop when stop button is clicked', async () => {
  const user = userEvent.setup();
  const onStop = vi.fn();

  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'active', selectedSourceId: 'juce-input-mic-1', selectedSourceName: 'Microphone (Realtek Audio)', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={onStop}
    />,
  );

  const stopButton = screen.getByRole('button', { name: /stop/i });
  await user.click(stopButton);
  expect(onStop).toHaveBeenCalledOnce();
});

test('renders source list with input devices when no source is selected', () => {
  render(
    <StandaloneSourceStrip
      bridge={{ ...readyBridgeStandalone, sourceList: fullSourceList, sourceState: { protocolVersion: 1, event: 'source.state', mode: 'InputDevice', state: 'stopped', selectedSourceId: '', selectedSourceName: '', code: '', message: '' } }}
      onSourceSelect={defaultSelectHandler}
      onSourceStop={defaultStopHandler}
    />,
  );
  expect(screen.getByRole('combobox', { name: /select source/i })).toBeInTheDocument();
});
