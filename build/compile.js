import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';
import { promisify } from 'util';

import { walk } from './walker.js';

const dirname = Path.dirname(fileURLToPath(import.meta.url));
const outputDirectory = Path.resolve(dirname, './_out');
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
  let target = 'x64';

  if (!FS.existsSync(outputDirectory)) {
    FS.mkdirSync(outputDirectory);
  }

  await compiler.initialize({ outputDirectory, target });

  console.log(`=> Compiling ${ target } -> ${ outputDirectory }`);

  let compiled = new Set();
  let outputs = [];

  await walk('scratch', async info => {
    info.output = compiler.getModuleOutputPath(info.filename);
    outputs.unshift(info.output);

    if (!info.imports.some(id => compiled.has(id))) {
      let newer = await isNewer(info.filename, info.output);
      if (!newer) {
        return;
      }
    }

    await compiler.compileModule(info.filename);

    compiled.add(info.name);

    if (typeof status === 'number' && status !== 0) {
      process.exit(status);
    }
  });

  console.log(`=> Linking`);
  await compiler.link({ outputs });
}
