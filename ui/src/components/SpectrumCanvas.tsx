import { useEffect, useRef } from 'react';
import type { SpectrumSnapshot } from '../bridge/protocol';

export interface SpectrumCanvasProps {
  snapshot: SpectrumSnapshot;
}

function yForValue(value: number, height: number) {
  const topPadding = 18;
  const bottomPadding = 24;
  const drawableHeight = Math.max(1, height - topPadding - bottomPadding);
  return topPadding + (1 - value) * drawableHeight;
}

export function SpectrumCanvas({ snapshot }: SpectrumCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rectWidth = canvas.clientWidth || 1;
    const rectHeight = canvas.clientHeight || 1;
    const pixelRatio = window.devicePixelRatio || 1;
    canvas.width = Math.max(1, Math.round(rectWidth * pixelRatio));
    canvas.height = Math.max(1, Math.round(rectHeight * pixelRatio));

    const context = canvas.getContext('2d');
    if (!context) return;

    context.setTransform?.(1, 0, 0, 1, 0, 0);
    context.clearRect(0, 0, canvas.width, canvas.height);
    context.scale?.(pixelRatio, pixelRatio);

    const width = rectWidth;
    const height = rectHeight;
    const bins = snapshot.bins;
    if (bins.length === 0) return;

    context.save();
    context.beginPath();
    context.moveTo(0, height - 18);

    const firstY = yForValue(bins[0].normalisedValue, height);
    context.lineTo(0, firstY);

    for (let index = 1; index < bins.length; index += 1) {
      const previousX = ((index - 1) / Math.max(1, bins.length - 1)) * width;
      const previousY = yForValue(bins[index - 1].normalisedValue, height);
      const currentX = (index / Math.max(1, bins.length - 1)) * width;
      const currentY = yForValue(bins[index].normalisedValue, height);
      const controlX = previousX + (currentX - previousX) * 0.5;
      context.bezierCurveTo(controlX, previousY, controlX, currentY, currentX, currentY);
    }

    context.lineTo(width, height - 18);
    context.closePath();
    context.fillStyle = 'rgba(39, 230, 202, 0.18)';
    context.fill();
    context.restore();

    context.save();
    context.beginPath();
    context.moveTo(0, firstY);
    for (let index = 1; index < bins.length; index += 1) {
      const previousX = ((index - 1) / Math.max(1, bins.length - 1)) * width;
      const previousY = yForValue(bins[index - 1].normalisedValue, height);
      const currentX = (index / Math.max(1, bins.length - 1)) * width;
      const currentY = yForValue(bins[index].normalisedValue, height);
      const controlX = previousX + (currentX - previousX) * 0.5;
      context.bezierCurveTo(controlX, previousY, controlX, currentY, currentX, currentY);
    }
    context.shadowBlur = 16;
    context.shadowColor = 'rgba(39, 230, 202, 0.34)';
    context.strokeStyle = 'rgba(139, 255, 235, 0.92)';
    context.lineWidth = 2;
    context.stroke();
    context.restore();
  }, [snapshot]);

  return (
    <canvas
      ref={canvasRef}
      data-testid="spectrum-canvas"
      aria-label="Live frequency spectrum"
      style={{
        display: 'block',
        width: '100%',
        height: '100%',
      }}
    />
  );
}
