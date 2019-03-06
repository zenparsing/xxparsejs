import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';
import { promisify } from 'util';

import { walk } from './walker.js';

const dirname = Path.dirname(fileURLToPath(import.meta.url));
const stat = promisify(FS.stat);

async function isNewer(first, second) {
  try {
    let [a, b] = await Promise.all([stat(first), stat(second)]);
    return a.mtimeMs >= b.mtimeMs;
  } catch (err) {
    if (err.code === 'ENOENT') {
      return true;
    }
    throw err;
  }
}

export async function compile(compiler) {
  let outputDirectory = Path.resolve(dirname, './_out');
  let target = 'x64';

  await compiler.initialize({ outputDirectory, target });

  console.log(`[Building ${ target } -> ${ outputDirectory }]`);

  let compiled = new Set();

  await walk('scratch', async info => {
    if (!info.imports.some(id => compiled.has(id))) {
      let outPath = compiler.getOutputPath(info);
      let newer = await isNewer(info.filename, outPath);
      if (!newer) {
        return;
      }
    }

    let status;
    if (info.isRoot) {
      status = await compiler.compileProgram(info.filename);
    } else {
      status = await compiler.compileModule(info.filename);
    }

    compiled.add(info.moduleName);

    if (typeof status === 'number' && status !== 0) {
      process.exit(status);
    }
  });

  await compiler.finalize();
}
