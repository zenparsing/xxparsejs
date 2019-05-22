import * as Path from 'path';
import * as FS from 'fs';

export function emptyDir(path) {
  if (!path.startsWith(process.cwd())) {
    throw new Error('Attempting to clear a folder outside of the working directory');
  }
  if (!FS.existsSync(path)) {
    return;
  }
  for (let child of FS.readdirSync(path)) {
    let childPath = Path.resolve(path, child);
    if (FS.statSync(childPath).isDirectory()) {
      emptyDir(childPath);
      FS.rmdirSync(childPath);
    } else {
      FS.unlinkSync(childPath);
    }
  }
}
