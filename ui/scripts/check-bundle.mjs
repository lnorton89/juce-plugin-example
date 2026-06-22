import { readdir, readFile } from 'node:fs/promises';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('../dist/', import.meta.url));
const forbidden = [
  /(?:src|href)=["']https?:\/\//i,
  /url\(["']?https?:\/\//i,
  /@import\s+["']https?:\/\//i,
  /fetch\(["']https?:\/\//i,
  /fonts\.googleapis\.com/i,
  /fonts\.gstatic\.com/i,
  /(?:^|[./])cdn\./i,
];

async function files(directory) {
  const entries = await readdir(directory, { withFileTypes: true });
  return (await Promise.all(entries.map(async (entry) => {
    const path = join(directory, entry.name);
    return entry.isDirectory() ? files(path) : [path];
  }))).flat();
}

const violations = [];
for (const path of await files(root)) {
  const content = await readFile(path, 'utf8');
  if (forbidden.some((pattern) => pattern.test(content))) violations.push(path);
}

if (violations.length > 0) {
  console.error(`Remote resource reference found in: ${violations.join(', ')}`);
  process.exit(1);
}
console.log('Bundle contains only local resource references.');
