import * as Path from 'path';
import * as FS from 'fs';
import { spawnSync, execSync } from 'child_process';
import Buffer from 'buffer';

// Locates the MSVS script that initializes required environment variables
function findEnvCmd() {
  let base = process.env['ProgramFiles(x86)'];
  if (!base) {
    throw new Error('Unable to find "Program Files (x86)" directory');
  }

  let vsRoot = Path.resolve(base, 'Microsoft Visual Studio');
  let versions = FS.readdirSync(vsRoot)
    .sort()
    .filter(dir => /\d+/.test(dir) || dir === 'Preview');

  if (versions.length === 0) {
    throw new Error('Unable to find Visual Studio version directories');
  }

  let version = versions.pop();
  let folder = Path.resolve(base, 'Microsoft Visual Studio', version);

  for (const child of FS.readdirSync(folder)) {
    let tryPath = Path.resolve(folder, child, 'VC/Auxiliary/Build/vcvarsall.bat');
    if (FS.existsSync(tryPath)) {
      return tryPath;
    }
  }

  throw new Error('Cannot find a Microsoft Visual Studio 2017 installation');
}

// Returns an object containing MSVS environment variables
function getEnvVars(outDir, hostArch, target) {
  let envSpec = target === hostArch ? target : `${ hostArch }_${ target }`;

  // Read variables from cached files if possible
  let cacheFile = Path.resolve(outDir, `./msvsenv_${ envSpec }.json`);
  try {
    return JSON.parse(FS.readFileSync(cacheFile, 'utf8'));
  } catch {}

  // Execute MSVS setup script and output all variables
  let cmd = findEnvCmd();
  let marker = '__MSVS_ENV_VARS__';
  let result = execSync(`"${ cmd }" ${ envSpec } & echo ${ marker } & set`, {
    encoding: 'utf8',
  });

  result = result.slice(result.indexOf(marker) + marker.length) + '\n';

  // Parse "set" output into object dictionary
  let vars = {};
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

export class MsvsCompiler {

  initialize({ outputDirectory, host, target }) {
    this._out = outputDirectory;
    this._env = getEnvVars(outputDirectory, host, target);
  }

  moduleOutputFile(name) {
    return name + '.obj';
  }

  linkOutputFile(name) {
    return name + '.exe';
  }

  hostArchitecture() {
    return getHostArch();
  }

  compileModule({ filename, output }) {
    this._cl(
      '/c',
      `/Fo${output}`,
      '/module:interface', filename
    );
  }

  link({ outputs }) {
    this._run('link', ['/nologo', ...outputs]);
  }

  _cl(...args) {
    this._run('cl', [
      '/std:c++17',
      '/EHsc',
      '/nologo',
      '/experimental:module',
      ...args
    ]);
  }

  _run(cmd, args) {
    let result = spawnSync(cmd, args, {
      stdio: 'pipe',
      cwd: this._out,
      env: this._env,
      encoding: 'utf8',
    });

    if (result.error) {
      throw result.error;
    }

    if (result.status !== 0) {
      console.log(`${ cmd } ${ args.join(' ') }`);
      console.log('');
      console.log(result.stdout);
      process.exit(result.status);
    }
  }

}
