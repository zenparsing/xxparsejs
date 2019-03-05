const path = require('path');
const { spawnSync } = require('child_process');

function fail(msg) {
  console.error(msg);
  process.exit(1);
}

function run(cmd, args = []) {
  let { status } = spawnSync(cmd, args, {
    stdio: 'inherit',
    cwd: path.resolve(__dirname, './_out'),
  });
  if (status) {
    process.exit(status);
  }
}

function compileModule(filename) {
  run('cl', [
    '/std:c++17',
    '/c',
    '/EHsc',
    '/nologo',
    '/experimental:module',
    '/module:interface',
    path.join('../../src/', filename),
  ]);
}

function compileProgram(filename) {
  run('cl', [
    '/std:c++17',
    '/EHsc',
    '/nologo',
    '/experimental:module',
    path.join('../../src/', filename),
  ]);
}

compileModule('BasicTypes.cpp');
compileModule('UnicodeData.cpp');
compileProgram('scratch.cpp');
