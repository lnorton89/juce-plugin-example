import { render, screen } from '@testing-library/react';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { expect, test } from 'vitest';
import App from '../App';
import { BridgeProvider } from '../bridge/BridgeProvider';

test('uses semantic regions, polite status, and keyboard-focusable recovery actions', () => {
  render(<BridgeProvider initialStatus={{ state: 'error', code: 'development_server_unavailable', message: 'dev server' }}><App /></BridgeProvider>);
  expect(screen.getByRole('banner')).toBeInTheDocument();
  expect(screen.getByRole('main')).toBeInTheDocument();
  expect(screen.getByRole('region', { name: 'Spectrum display' })).toBeInTheDocument();
  expect(screen.getByRole('contentinfo')).toBeInTheDocument();
  expect(screen.getByRole('status')).toHaveAttribute('aria-live', 'polite');
  const retry = screen.getByRole('button', { name: 'Retry connection' });
  const copy = screen.getByRole('button', { name: 'Copy diagnostics' });
  retry.focus();
  expect(retry).toHaveFocus();
  copy.focus();
  expect(copy).toHaveFocus();
});

test('decorative motif is hidden and reduced-motion rule is shipped', () => {
  const { container } = render(<BridgeProvider><App /></BridgeProvider>);
  expect(container.querySelector('.spectral-motif')).toHaveAttribute('aria-hidden', 'true');
  const motifCss = readFileSync(resolve(process.cwd(), 'src/styles/spectral-motif.css'), 'utf8');
  expect(motifCss).toContain('prefers-reduced-motion: reduce');
  expect(motifCss).toContain('animation: none');
});

test('long literal diagnostics remain constrained within the stage', () => {
  render(<BridgeProvider initialStatus={{ state: 'error', code: '<code>', message: '<script>bad()</script>'.repeat(30) }}><App /></BridgeProvider>);
  const details = screen.getByLabelText('Diagnostic details');
  expect(details).toHaveTextContent('<script>bad()</script>');
  expect(details).toHaveStyle({ maxWidth: '100%', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' });
  expect(document.querySelector('script')).toBeNull();
});
