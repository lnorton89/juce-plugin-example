import { describe, expect, test } from 'vitest';

const modules = import.meta.glob('../**/*.{ts,tsx,css}', { eager: true, query: '?raw', import: 'default' });
const source = Object.entries(modules)
  .filter(([path]) => !path.includes('/test/') && !path.endsWith('.test.tsx'))
  .map(([, value]) => String(value))
  .join('\n');

describe('offline and safe rendering contract', () => {
  test('contains no remote resources, analytics, or network fonts', () => {
    expect(source).not.toMatch(/https?:\/\//i);
    expect(source).not.toMatch(/fonts\.(?:googleapis|gstatic)\.com|google-analytics|segment\.com/i);
    expect(source).not.toMatch(/(?:src|url\()["']?https?:\/\//i);
  });

  test('does not inject diagnostic markup', () => {
    expect(source).not.toContain('dangerouslySetInnerHTML');
  });
});
