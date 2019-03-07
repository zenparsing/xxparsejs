import * as Path from 'path';
import * as FS from 'fs';
import { spawnSync, execSync } from 'child_process';
import Buffer from 'buffer';

import { compile } from './compile.js';

// Gets the host architecture from environment variables
function getHostArch() {
  for (let key of Object.keys(process.env)) {
    if (key.startsWith('PROCESSOR_ARCH')) {
      return process.env[key] === 'AMD64' ? 'x64' : 'x86';
    }
  }
  throw new Error('Unable to determine host processor architecture');
}

// Locates the MSVS script that initializes required environment variables
function findEnvCmd() {
  let base = process.env['ProgramFiles(x86)'];
  if (!base) {
    throw new Error('Unable to find "Program Files (x86)" directory');
  }

  let folder = Path.resolve(base, 'Microsoft Visual Studio/2017');
  for (const child of FS.readdirSync(folder)) {
    let tryPath = Path.resolve(folder, child, 'VC/Auxiliary/Build/vcvarsall.bat');
    if (FS.existsSync(tryPath)) {
      return tryPath;
    }
  }

  throw new Error('Cannot find a Microsoft Visual Studio 2017 installation');
}

// Returns an object containing MSVS environment variables
function getEnvVars(outDir, target) {
  let hostArch = getHostArch();
  let envSpec = target === hostArch ? target : `${ hostArch }_${ target }`;

  // Read variables from cached files if possible
  let cacheFile = Path.resolve(outDir, `./msvsenv_${ envSpec }.json`);
  try {
    return JSON.parse(FS.readFileSync(cacheFile, 'utf8'));
  } catch {}

  // Execute MSVS setup script and output all variables
  let cmd = findEnvCmd();
  let marker = '__MSVS_ENV_VARS__';
  let result = execSync(`"${ cmd }" ${ envSpec } & echo ${ marker } & set`, { encoding: 'utf8' });
  let vars = {};

  result = result.slice(result.indexOf(marker) + marker.length) + '\n';

  // Parse "set" output into object dictionary
  let key = null;
  let start = 0;
  for (let i = 0; i < result.length; ++i) {
    let c = result[i];
    if (c === '\n') {
      if (!key) {
        key = result.slice(start, i);
        start = i;
      }
      key = key.trim();
      if (key) {
        vars[key] = result.slice(start, i).trim();
        start = i + 1;
        key = null;
      }
    } else if (c === '=') {
      if (!key) {
        key = result.slice(start, i);
        start = i + 1;
      }
    }
  }

  // Cache environment variables
  FS.writeFileSync(cacheFile, JSON.stringify(vars), 'utf8');

  return vars;
}

class Compiler {

  initialize({ outputDirectory, target }) {
    this._out = outputDirectory;
    this._env = getEnvVars(outputDirectory, target);
  }

  getModuleOutputPath(filename) {
    return Path.resolve(this._out, Path.basename(filename, '.cpp') + '.obj');
  }

  compileModule(filename) {
    this._cl('/c', '/module:interface', filename);
  }

  link({ outputs }) {
    this._run('link', ['/nologo', ...outputs ]);
  }

  _cl(...args) {
    this._run('cl', [
      '/std:c++latest',
      '/EHsc',
      '/nologo',
      '/experimental:module',
      ...args
    ]);
  }

  _run(cmd, args) {
    let result = spawnSync(cmd, args, {
      stdio: 'inherit',
      cwd: this._out,
      env: this._env,
      encoding: 'utf8',
    });

    if (result.error) {
      throw result.error;
    }

    if (result.status !== 0) {
      process.exit(result.status);
    }
  }

}

compile(new Compiler());
