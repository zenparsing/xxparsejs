import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';
import { promisify } from 'util';

const dirname = Path.dirname(fileURLToPath(import.meta.url));
const sourceDirectory = Path.resolve(dirname, '../src');
const outputDirectory = Path.resolve(dirname, './_out');
const importPattern = /(?:^|\n)import ([^;\n\r]+)/g;

const AFS = {
  exists: promisify(FS.exists),
  stat: promisify(FS.stat),
  mkdir: promisify(FS.mkdir),
  readFile: promisify(FS.readFile),
}

async function isNewer(first, second) {
  try {
    let [a, b] = await Promise.all([AFS.stat(first), AFS.stat(second)]);
    return a.mtimeMs >= b.mtimeMs;
  } catch (err) {
    if (err.code === 'ENOENT') {
      return true;
    }
    throw err;
  }
}

export async function compile(main, compiler) {
  let target = 'x64';

  if (!await AFS.exists(outputDirectory)) {
    await AFS.mkdir(outputDirectory);
  }

  await compiler.initialize({ outputDirectory, target });

  console.log(`=> Compiling ${ target } -> ${ outputDirectory }`);

  let compiled = new Set();
  let outputs = [];

  await walk(main, async info => {
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
  if (compiled.size > 0) {
    await compiler.link({ outputs });
  }
}

async function getImportsFromFile(filename) {
  let source = await AFS.readFile(filename, 'utf8');
  let imports = [];
  for (let match; match = importPattern.exec(source);) {
    imports.push(match[1]);
  }
  return imports;
}

async function resolveModule(name) {
  let dir = sourceDirectory;
  // Modules starting with "test." are in the test dir
  if (name.startsWith('test.')) {
    dir = Path.resolve(dirname, '../');
  }
  return Path.resolve(dir, name.replace('.', '/') + '.cpp');
}

export async function walk(name, callback, moduleMap = new Map()) {
  let state = moduleMap.get(name);

  switch (state) {
    case 'visiting': throw new Error(`Cycle detected for module ${ name }`);
    case 'visited': return;
  }

  moduleMap.set(name, 'visiting');

  let filename = await resolveModule(name);
  let imports = await getImportsFromFile(filename);

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
