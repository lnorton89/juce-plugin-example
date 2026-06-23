import { render, screen } from '@testing-library/react';
import { beforeEach, describe, expect, test, vi } from 'vitest';
import spectrumSnapshot from '../../../tests/fixtures/bridge/spectrum-snapshot-v1.json';
import { SpectrumCanvas } from './SpectrumCanvas';

const calls = {
  fill: vi.fn(),
  stroke: vi.fn(),
  bezierCurveTo: vi.fn(),
  lineTo: vi.fn(),
  shadowBlur: 0,
  fillStyle: '',
  strokeStyle: '',
};

beforeEach(() => {
  vi.clearAllMocks();
  calls.shadowBlur = 0;
  calls.fillStyle = '';
  calls.strokeStyle = '';
  Object.defineProperty(HTMLCanvasElement.prototype, 'clientWidth', { configurable: true, value: 640 });
  Object.defineProperty(HTMLCanvasElement.prototype, 'clientHeight', { configurable: true, value: 280 });
  HTMLCanvasElement.prototype.getContext = vi.fn(() => ({
    clearRect: vi.fn(),
    beginPath: vi.fn(),
    moveTo: vi.fn(),
    lineTo: calls.lineTo,
    bezierCurveTo: calls.bezierCurveTo,
    closePath: vi.fn(),
    fill: calls.fill,
    stroke: calls.stroke,
    save: vi.fn(),
    restore: vi.fn(),
    scale: vi.fn(),
    setTransform: vi.fn(),
    set shadowBlur(value: number) { calls.shadowBlur = value; },
    set fillStyle(value: string) { calls.fillStyle = value; },
    set strokeStyle(value: string) { calls.strokeStyle = value; },
  } as unknown as CanvasRenderingContext2D));
});

describe('SpectrumCanvas', () => {
  test('renders one canvas and no per-bin DOM elements', () => {
    const manyBins = {
      ...spectrumSnapshot,
      bins: Array.from({ length: 160 }, (_, index) => ({
        frequencyHz: 20 + index,
        decibels: -72 + (index % 24),
        normalisedValue: index / 159,
      })),
    };

    const { container } = render(<SpectrumCanvas snapshot={manyBins} />);

    expect(screen.getByTestId('spectrum-canvas')).toBeInTheDocument();
    expect(container.querySelectorAll('canvas')).toHaveLength(1);
    expect(container.querySelectorAll('[data-spectrum-bin]')).toHaveLength(0);
  });

  test('draws a smooth filled curve with dark-panel styling hooks', () => {
    render(<SpectrumCanvas snapshot={spectrumSnapshot} />);

    expect(calls.bezierCurveTo).toHaveBeenCalled();
    expect(calls.fill).toHaveBeenCalled();
    expect(calls.stroke).toHaveBeenCalled();
    expect(calls.shadowBlur).toBeGreaterThan(0);
    expect(calls.fillStyle).toContain('rgba');
    expect(calls.strokeStyle).toContain('rgba');
  });
});
