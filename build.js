import * as Path from 'path';
import { fileURLToPath } from 'url';
import { spawnSync } from 'child_process';

import { compile } from './tools/compiler/compile.js';
import { parseArgs } from './tools/parse-args.js';

const dirname = Path.dirname(fileURLToPath(import.meta.url));
const sourceDirectory = Path.resolve(dirname, './src');
const outputDirectory = Path.resolve(dirname, './build');

async function resolveModule(name) {
  let dir = sourceDirectory;
  // Modules starting with "test." are in the test dir
  if (name.startsWith('test.')) {
    dir = dirname;
  }
  return Path.resolve(dir, name.replace('.', '/') + '.cpp');
}

let args = parseArgs({
  flags: ['--run'],
  alias: {
    '-r': '--run',
  },
})

let main = args[0];
if (!main) {
  console.log('No main module specified');
  process.exit(1);
}

compile({
  main,
  outputDirectory,
  resolveModule,
  log(msg) { console.log(`=> ${ msg }`); },
  compiler: 'msvs',
}).then(filename => {
  if (args.named.has('--run')) {
    console.log(`=> Running ${ filename }`);
    let { status } = spawnSync(filename, [], {
      stdio: 'inherit',
    });
    console.log(`=> Process exited with status: ${ status }`);
  }
});
