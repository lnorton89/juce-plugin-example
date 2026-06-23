import { defineConfig } from 'vitest/config';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));

export default defineConfig({
  test: {
    root: resolve(__dirname, '..', 'tests', 'worker'),
    include: ['**/*.test.ts'],
  },
  resolve: {
    alias: {
      '@worker': resolve(__dirname, 'src'),
    },
  },
});
