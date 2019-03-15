export function parseArgs(options = {}) {
  let {
    argv = process.argv.slice(2),
    flags = [],
    alias = {}
  } = options;

  let flagSet = new Set(flags);
  let aliasMap = new Map(Object.keys(alias).map(k => [k, alias[k]]));
  let list = [];
  let map = new Map();
  let key = null;

  for (let part of argv) {
    if (part.startsWith('-')) {
      if (aliasMap.has(part)) {
        part = aliasMap.get(part);
      }
      if (flagSet.has(part)) {
        map.set(part, true);
        key = null;
      } else {
        map.set(key = part, undefined);
      }
    } else if (key) {
      map.set(key, part);
      key = null;
    } else {
      list.push(part);
    }
  }

  list.named = map;
  return list;
}
