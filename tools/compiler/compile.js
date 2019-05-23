import * as FS from 'fs';
import * as Path from 'path';
import { fileURLToPath } from 'url';
import { promisify } from 'util';
import { spawnSync } from 'child_process';

import { MsvsCompiler } from './msvs.js';

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

function getHostArch() {
  for (let key of Object.keys(process.env)) {
    if (key.startsWith('PROCESSOR_ARCH')) {
      return process.env[key] === 'AMD64' ? 'x64' : 'x86';
    }
  }
  throw new Error('Unable to determine host processor architecture');
}

function getCompiler(type) {
  switch (type) {
    case 'msvs': return new MsvsCompiler();
    default: throw new Error(`Unsupported compiler "${ type }"`);
  }
}

export async function compile(options) {
  let compiler = getCompiler(options.compiler);
  let host = getHostArch();
  let {
    target = host,
    log = () => {},
    run = false,
    main,
    outputDirectory,
    resolveModule,
  } = options;

  if (!await AFS.exists(outputDirectory)) {
    await AFS.mkdir(outputDirectory);
  }

  log('Initializing environment');

  await compiler.initialize({
    outputDirectory,
    host,
    target,
  });

  log(`Compiling ${ target } -> ${ outputDirectory }`);

  let compiled = new Set();
  let outputs = [];

  await walk(main, resolveModule, async info => {
    info.output = compiler.moduleOutputFile(info.name);
    info.main = (info.name === main);

    outputs.unshift(info.output);

    if (!info.imports.some(id => compiled.has(id))) {
      let outPath = Path.resolve(outputDirectory, info.output);
      let newer = await isNewer(info.filename, outPath);
      if (!newer) {
        return;
      }
    }

    log(`${ info.name } -> ${ info.output }`);
    await compiler.compileModule(info);

    compiled.add(info.name);

    if (typeof status === 'number' && status !== 0) {
      process.exit(status);
    }
  });

  let linkOutput = Path.resolve(
    outputDirectory,
    compiler.linkOutputFile(main));

  log(`Linking ${ target } -> ${ linkOutput }`);

  if (compiled.size > 0 || !await AFS.exists(linkOutput)) {
    await compiler.link({ outputs });
  }

  return linkOutput;
}

async function getImportsFromFile(filename) {
  let source = await AFS.readFile(filename, 'utf8');
  let imports = [];
  for (let match; match = importPattern.exec(source);) {
    imports.push(match[1]);
  }
  return imports;
}

export async function walk(name, resolveModule, callback, moduleMap = new Map()) {
  let state = moduleMap.get(name);

  switch (state) {
    case 'visiting': throw new Error(`Cycle detected for module ${ name }`);
    case 'visited': return;
  }

  if (name.startsWith('std.')) {
    moduleMap.set(name, 'visited');
    return;
  }

  moduleMap.set(name, 'visiting');

  let filename = await resolveModule(name);
  let imports = await getImportsFromFile(filename);

  for (let importName of imports) {
    await walk(importName, resolveModule, callback, moduleMap);
  }

  await callback({
    name,
    filename,
    imports,
    output: '',
    main: false,
  });

  moduleMap.set(name, 'visited');
}
