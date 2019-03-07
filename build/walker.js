import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';

const sourceDir = Path.resolve(fileURLToPath(import.meta.url), '../../src');
const importPattern = /(?:^|\n)import ([^;\n\r]+)/g;

export async function walk(name, callback, moduleMap = new Map()) {
  let state = moduleMap.get(name);

  switch (state) {
    case 'visiting': throw new Error('Module cycle detected');
    case 'visited': return;
  }

  moduleMap.set(name, 'visiting');

  let filename = Path.resolve(sourceDir, name + '.cpp');
  let source = FS.readFileSync(filename, 'utf8');

  let imports = [];
  for (let match; match = importPattern.exec(source);) {
    imports.push(match[1]);
  }

  for (let importName of imports) {
    await walk(importName, callback, moduleMap);
  }

  await callback({
    name,
    filename,
    imports,
    output: '',
  });

  moduleMap.set(name, 'visited');
}
