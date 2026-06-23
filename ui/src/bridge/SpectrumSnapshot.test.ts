import { describe, expect, test } from 'vitest';
import fixture from '../../../tests/fixtures/bridge/spectrum-snapshot-v1.json';
import { parseSpectrumSnapshot, protocolVersion, spectrumSnapshotEvent } from './protocol';

describe('spectrum snapshot protocol', () => {
  test('valid fixture parses with bounded metadata and bins', () => {
    const snapshot = parseSpectrumSnapshot(fixture);

    expect(spectrumSnapshotEvent).toBe('spectrum.snapshot');
    expect(snapshot).not.toBeNull();
    expect(snapshot?.protocolVersion).toBe(protocolVersion);
    expect(snapshot?.sequence).toBe(42);
    expect(snapshot?.profile).toBe('Musical');
    expect(snapshot?.sampleRate).toBe(48000);
    expect(snapshot?.fftSize).toBe(4096);
    expect(snapshot?.minFrequencyHz).toBe(20);
    expect(snapshot?.maxFrequencyHz).toBe(20000);
    expect(snapshot?.minDecibels).toBe(-96);
    expect(snapshot?.maxDecibels).toBe(0);
    expect(snapshot?.bins).toHaveLength(3);
    expect(snapshot?.bins[1]).toEqual({ frequencyHz: 1000, decibels: -12, normalisedValue: 0.875 });
  });

  test.each([
    ['wrong protocol version', { protocolVersion: 2 }],
    ['invalid profile', { profile: 'Experimental' }],
    ['non-finite sample rate', { sampleRate: Number.NaN }],
    ['malformed bin', { bins: [{ frequencyHz: 20, decibels: -12, normalisedValue: 1.2 }] }],
    ['oversized bins', { bins: Array.from({ length: 257 }, (_, index) => ({ frequencyHz: 20 + index, decibels: -48, normalisedValue: 0.5 })) }],
  ])('rejects %s', (_, patch) => {
    expect(parseSpectrumSnapshot({ ...fixture, ...patch })).toBeNull();
  });
});
