import { render, screen } from '@testing-library/react';
import { expect, test } from 'vitest';
import App from './App';
import { BridgeProvider } from './bridge/BridgeProvider';

test('renders the honest connecting shell', () => {
  render(<BridgeProvider><App /></BridgeProvider>);
  expect(screen.getByRole('heading', { name: 'LumaScope' })).toBeInTheDocument();
  expect(screen.getByRole('region', { name: 'Spectrum display' })).toBeInTheDocument();
  expect(screen.getByRole('status')).toHaveTextContent('Connecting to audio engine');
});
