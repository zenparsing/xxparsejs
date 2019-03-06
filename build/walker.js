import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';

const sourceDir = Path.resolve(fileURLToPath(import.meta.url), '../../src');
const importPattern = /(?:^|\n)import ([^;\n\r]+)/g;

export async function walk(moduleName, callback, moduleMap = new Map()) {
  let isRoot = moduleMap.size === 0;
  let state = moduleMap.get(moduleName);

  switch (state) {
    case 'visiting': throw new Error('Module cycle detected');
    case 'visited': return;
  }

  moduleMap.set(moduleName, 'visiting');

  let filename = Path.resolve(sourceDir, moduleName + '.cpp');
  let source = FS.readFileSync(filename, 'utf8');

  let imports = [];
  for (let match; match = importPattern.exec(source);) {
    imports.push(match[1]);
  }

  for (let name of imports) {
    await walk(name, callback, moduleMap);
  }

  await callback({ filename, moduleName, isRoot, imports });

  moduleMap.set(moduleName, 'visited');
}
