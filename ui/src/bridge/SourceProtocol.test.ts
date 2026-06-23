import { describe, expect, test } from 'vitest';
import {
  parseSourceList,
  parseSourceState,
  protocolVersion,
  sourceListEvent,
  sourceStateEvent,
  sourceSelectEvent,
  sourceStopEvent,
} from './protocol';

describe('source protocol constants', () => {
  test('event identifier strings are stable', () => {
    expect(sourceListEvent).toBe('source.list');
    expect(sourceStateEvent).toBe('source.state');
    expect(sourceSelectEvent).toBe('source.select');
    expect(sourceStopEvent).toBe('source.stop');
  });
});

describe('parseSourceList', () => {
  const validList = {
    protocolVersion: 1,
    event: 'source.list',
    inputDevices: [
      { id: 'juce-input-mic-1', displayName: 'Microphone (Realtek Audio)', mode: 'InputDevice' },
    ],
    systemOutputs: [
      { id: 'wasapi-speaker-1', displayName: 'Speakers (Realtek Audio)', mode: 'SystemOutput' },
    ],
  };

  test('parses valid source list with both device types', () => {
    const result = parseSourceList(validList);
    expect(result).not.toBeNull();
    expect(result!.protocolVersion).toBe(protocolVersion);
    expect(result!.event).toBe('source.list');
    expect(result!.inputDevices).toHaveLength(1);
    expect(result!.inputDevices[0].id).toBe('juce-input-mic-1');
    expect(result!.inputDevices[0].displayName).toBe('Microphone (Realtek Audio)');
    expect(result!.inputDevices[0].mode).toBe('InputDevice');
    expect(result!.systemOutputs).toHaveLength(1);
    expect(result!.systemOutputs[0].id).toBe('wasapi-speaker-1');
    expect(result!.systemOutputs[0].mode).toBe('SystemOutput');
  });

  test('parses empty source lists', () => {
    const empty = { protocolVersion: 1, event: 'source.list', inputDevices: [], systemOutputs: [] };
    expect(parseSourceList(empty)).not.toBeNull();
    expect(parseSourceList(empty)!.inputDevices).toHaveLength(0);
    expect(parseSourceList(empty)!.systemOutputs).toHaveLength(0);
  });

  test('rejects wrong protocol version', () => {
    expect(parseSourceList({ ...validList, protocolVersion: 2 })).toBeNull();
  });

  test('rejects unknown source mode', () => {
    const badMode = {
      ...validList,
      inputDevices: [{ id: 'x', displayName: 'X', mode: 'UnknownMode' }],
    };
    expect(parseSourceList(badMode)).toBeNull();
  });

  test('rejects oversized ID string (>256 chars)', () => {
    const longId = 'a'.repeat(257);
    const oversize = {
      ...validList,
      inputDevices: [{ id: longId, displayName: 'Test', mode: 'InputDevice' }],
    };
    expect(parseSourceList(oversize)).toBeNull();
  });

  test('rejects oversized displayName (>256 chars)', () => {
    const longName = 'b'.repeat(257);
    const oversize = {
      ...validList,
      inputDevices: [{ id: 'test-id', displayName: longName, mode: 'InputDevice' }],
    };
    expect(parseSourceList(oversize)).toBeNull();
  });

  test('rejects null or non-object inputs', () => {
    expect(parseSourceList(null)).toBeNull();
    expect(parseSourceList(undefined)).toBeNull();
    expect(parseSourceList('string')).toBeNull();
    expect(parseSourceList(42)).toBeNull();
  });

  test('rejects unknown event field', () => {
    expect(parseSourceList({ ...validList, event: 'unknown.event' })).toBeNull();
  });

  test('rejects missing arrays', () => {
    expect(parseSourceList({ protocolVersion: 1, event: 'source.list' })).toBeNull();
  });
});

describe('parseSourceState', () => {
  const validActiveState = {
    protocolVersion: 1,
    event: 'source.state',
    mode: 'InputDevice',
    state: 'active',
    selectedSourceId: 'juce-input-mic-1',
    selectedSourceName: 'Microphone (Realtek Audio)',
    code: '',
    message: '',
  };

  test('parses valid active source state', () => {
    const result = parseSourceState(validActiveState);
    expect(result).not.toBeNull();
    expect(result!.protocolVersion).toBe(protocolVersion);
    expect(result!.event).toBe('source.state');
    expect(result!.mode).toBe('InputDevice');
    expect(result!.state).toBe('active');
    expect(result!.selectedSourceId).toBe('juce-input-mic-1');
    expect(result!.selectedSourceName).toBe('Microphone (Realtek Audio)');
    expect(result!.code).toBe('');
    expect(result!.message).toBe('');
  });

  test('parses stopped state with empty source fields', () => {
    const stopped = {
      protocolVersion: 1,
      event: 'source.state',
      mode: 'InputDevice',
      state: 'stopped',
      selectedSourceId: '',
      selectedSourceName: '',
      code: '',
      message: '',
    };
    const result = parseSourceState(stopped);
    expect(result).not.toBeNull();
    expect(result!.state).toBe('stopped');
    expect(result!.selectedSourceId).toBe('');
  });

  test('parses silent state without error fields', () => {
    const silent = {
      ...validActiveState,
      state: 'silent',
    };
    const result = parseSourceState(silent);
    expect(result).not.toBeNull();
    expect(result!.state).toBe('silent');
    expect(result!.code).toBe('');
  });

  test('parses error state with code and message', () => {
    const errorState = {
      ...validActiveState,
      state: 'error',
      code: 'source_lost',
      message: 'The selected device was disconnected.',
    };
    const result = parseSourceState(errorState);
    expect(result).not.toBeNull();
    expect(result!.state).toBe('error');
    expect(result!.code).toBe('source_lost');
    expect(result!.message).toBe('The selected device was disconnected.');
  });

  test('rejects wrong protocol version', () => {
    expect(parseSourceState({ ...validActiveState, protocolVersion: 2 })).toBeNull();
  });

  test('rejects unknown mode value', () => {
    expect(parseSourceState({ ...validActiveState, mode: 'InvalidMode' })).toBeNull();
  });

  test('rejects invalid state enum', () => {
    expect(parseSourceState({ ...validActiveState, state: 'invalid_state' })).toBeNull();
  });

  test('rejects oversized code (>64 chars)', () => {
    expect(parseSourceState({ ...validActiveState, state: 'error', code: 'c'.repeat(65), message: 'test' })).toBeNull();
  });

  test('rejects oversized message (>256 chars)', () => {
    expect(parseSourceState({ ...validActiveState, state: 'error', code: 'err', message: 'm'.repeat(257) })).toBeNull();
  });

  test('rejects oversized selectedSourceId (>256 chars)', () => {
    expect(parseSourceState({ ...validActiveState, selectedSourceId: 'i'.repeat(257) })).toBeNull();
  });

  test('rejects null or non-object inputs', () => {
    expect(parseSourceState(null)).toBeNull();
    expect(parseSourceState(undefined)).toBeNull();
    expect(parseSourceState(42)).toBeNull();
    expect(parseSourceState('string')).toBeNull();
  });

  test('rejects unknown event field', () => {
    expect(parseSourceState({ ...validActiveState, event: 'wrong.event' })).toBeNull();
  });
});
