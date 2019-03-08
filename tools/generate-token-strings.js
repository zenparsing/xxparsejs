const FS = require('fs');
const Path = require('path');

const OUT_PATH = Path.resolve(__dirname, '../test/TokenStrings.cpp');
const IN_PATH = Path.resolve(__dirname, '../src/Token.cpp');

let typesFile = FS.readFileSync(IN_PATH, 'utf8');

function matchAll(text, re) {
  let out = [];
  re.lastIndex = 0;
  for (let m; m = re.exec(text);) {
    out.push(m);
  }
  return out;
}

let identifiers = matchAll(typesFile, /\n[ \t]+(\w+),/g)
  .map(m => m[1])
  .filter(id => !id.endsWith('_begin') && !id.endsWith('_end'));

let code = `\
export module test.TokenStrings;

import Token;

export template<typename T>
T& operator<<(T& out, Token t) {
  switch (t) {
${ identifiers.map(id => {
  return `    case Token::${ id }: return out << "${ id }";`;
}).join('\n') }
    default: return out << "?";
  }
}
`;

FS.writeFileSync(OUT_PATH, code, 'utf8');
