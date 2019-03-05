/*

=== Unicode Test Generator for ES Parsers ===

When parsing ES, there are three Unicode character classifications
that we are interested in:

- Whitespace:  In addition to the whitespace characters explicitly
  defined in the ES6 specification, we also need to recognize any
  character with a Unicode general category of 'Zs'.  The predefined
  whitespace characters are:

  - U+0009 <TAB> [\t]
  - U+000B <VT> [\v]
  - U+000C <FF> [\f]
  - U+0020 <SP> [ ]
  - U+00A0 <NBSP>
  - U+FEFF <BOM>

  In order to find the other whitespace characters, we download the
  text file located at http://www.unicode.org/Public/UNIDATA/PropList.txt
  and find all of the character ranges with the 'White_Space' property
  and whose general category is 'Zs'.

- IdentifierStart and IdentifierPart:  In addition to the identifier
  characters explicitly defined in the ES6 specification, we also need
  to recognize characters with a Unicode derived property of 'ID_Start'
  or 'ID_Continue'.  The predefined identifier characters are:

  - U+0024 [$] IdentifierStart
  - U+005F [_] IdentifierStart
  - U+200C <ZWNJ> IdentifierPart
  - U+200D <ZWJ> IdentifierPart

  In order to find the other identifier characters, we download the
  text file located at http://www.unicode.org/Public/UNIDATA/DerivedCoreProperties.txt
  and find all of the character ranges with a Unicode derived property
  of 'ID_Start' or 'ID_Continue' which can be represented in UTF-16 as
  a single code unit.

After obtaining the list of valid code points for whitespace and identifiers,
we build a list of ranges and output code

*/

const Path = require('path');
const HTTP = require('http');
const FS = require('fs');

let unicodeVersion = '';
let unicodeDate = '';
let wsRanges;
let idRanges;

const BASE_URL = 'http://www.unicode.org/Public/UNIDATA';
const PROP_LIST_URL = `${ BASE_URL }/PropList.txt`;
const DERIVED_PROPS_URL = `${ BASE_URL }/DerivedCoreProperties.txt`;

const WORK_FOLDER = Path.resolve(__dirname, '_unicode');
const PROP_LIST_PATH = Path.resolve(WORK_FOLDER, 'PropList.txt');
const DERIVED_PROPS_PATH = Path.resolve(WORK_FOLDER, 'DerivedCoreProperties.txt');
const OUT_PATH = Path.resolve(__dirname, '../src/UnicodeData.cpp');

// Whitespace
const CP_TAB = 0x09;
const CP_VT = 0x0b;
const CP_FF = 0x0c;
const CP_SP = 0x20;
const CP_NBSP = 0xa0;
const CP_BOM = 0xfeff;

// Identifiers
const CP_DOLLAR = 0x24;
const CP_UNDERSCORE = 0x5f;
const CP_ZWNJ = 0x200c;
const CP_ZWJ = 0x200d;

// Code Point Types
const WHITE_SPACE = 1;
const ID_START = 2;
const ID_CONT = 3;

function log(msg) {
  console.log('...' + msg);
}

function fail(err) {
  log('Oops!');
  console.error(err);
}

function complete() {
  log('Finished.');
}

function downloadDatabase(url, filePath, callback) {
  return new Promise((resolve, reject) => {
    let filename = Path.basename(filePath);

    if (FS.existsSync(filePath)) {
      log(filename + ' database already downloaded');
      return resolve();
    }

    log('Downloading ' + filename + ' database');

    HTTP.get(url, response => {
      let out = FS.createWriteStream(filePath);
      response.pipe(out);
      response.on('end', resolve);
    }).on('error', reject);
  });
}

function getUnicodeVersion(data) {
  let version = /^#[^\n\d]*(\d+(?:\.\d+)*)/;
  let date = /\n#\s*Date:\s*(.+)/;
  let match;

  if (match = version.exec(data)) {
    unicodeVersion = match[1];
  }

  if (match = date.exec(data)) {
    unicodeDate = match[1];
  }
}

function foldRanges(map) {
  let ranges = [];
  let range;

  Object.keys(map).sort((a, b) => +(a) - b).forEach(val => {
    // Convert val to a number
    val = +val;

    if (range && val === range.to + 1 && map[val] === range.type) {
      range.to = val;
    } else {
      range = {
        from: val,
        to: val,
        type: map[val],
      };
      ranges.push(range);
    }
  });

  return ranges;
}

function loadWhiteSpaceChars() {
  let wsMap = {};

  // Load pre-defined whitespace code points
  wsMap[CP_TAB] =
  wsMap[CP_VT] =
  wsMap[CP_FF] =
  wsMap[CP_SP] =
  wsMap[CP_NBSP] =
  wsMap[CP_BOM] = WHITE_SPACE;

  log('Opening PropList.txt database');

  let propList = FS.readFileSync(PROP_LIST_PATH, {
    encoding: 'utf8'
  });

  let pattern = /\n([a-fA-F0-9\.]+)\s+;\s*White_Space\s*#\s*Zs/g;
  let match;

  getUnicodeVersion(propList);

  log('Unicode ' + unicodeVersion + ' | ' + unicodeDate);
  log('Scanning PropList.txt for White_Space (Zs)');

  while (match = pattern.exec(propList)) {
    let range = match[1].split('..').map(val => parseInt(val, 16));

    if (range.length === 1) {
      range[1] = range[0];
    }

    for (let code = range[0]; code <= range[1]; ++code) {
      wsMap[code] = WHITE_SPACE;
    }
  }

  wsRanges = foldRanges(wsMap);
}

function loadIdentChars() {
  let idMap = {};

  // Load pre-defined identifier code points
  idMap[CP_DOLLAR] = ID_START;
  idMap[CP_UNDERSCORE] = ID_START;
  idMap[CP_ZWNJ] = ID_CONT;
  idMap[CP_ZWJ] = ID_CONT;

  log('Opening DerivedCoreProperties.txt database');

  let propList = FS.readFileSync(DERIVED_PROPS_PATH, {
    encoding: 'utf8'
  });

  let pattern = /\n([a-fA-F0-9\.]+)\s+;\s*ID_(Start|Continue)\s+/g;
  let match;

  log('Scanning DerivedCoreProperties.txt for ID_Start and ID_Continue');

  while (match = pattern.exec(propList)) {
    let range = match[1].split('..').map(val => parseInt(val, 16));

    if (range.length === 1) {
      range[1] = range[0];
    }

    for (let code = range[0]; code <= range[1]; ++code) {
      if (idMap[code] !== ID_START) {
        idMap[code] = match[2] === 'Start' ? ID_START : ID_CONT;
      }
    }
  }

  idRanges = foldRanges(idMap);
}

(async function main() {

  log('Unicode Test Generator for ES6 Parsers');

  if (!FS.existsSync(WORK_FOLDER)) {
    log('Creating work folder');
    FS.mkdirSync(WORK_FOLDER);
  }

  await downloadDatabase(PROP_LIST_URL, PROP_LIST_PATH);
  await downloadDatabase(DERIVED_PROPS_URL, DERIVED_PROPS_PATH);

  loadWhiteSpaceChars();
  loadIdentChars();

  log('Generating C++ code');

  let idInit = '{\n' +
    idRanges.map(range => {
      let start = range.type === ID_START ? 'true' : 'false';
      return `  {${ range.from }, ${ range.to - range.from }, ${ start }},\n`;
    }).join('') +
  '}';

  let wsInit = '{\n' +
    wsRanges.map(range => {
      return `  {${ range.from }, ${ range.to - range.from }},\n`;
    }).join('') +
  '}';

  let out = '// Unicode ' + unicodeVersion + ' | ' + unicodeDate + '\n' +
    'export module UnicodeData;\n\n' +
    'import BasicTypes;\n\n' +
    'export struct identifier_span {\n  uint32 id;\n  uint16 length;\n  bool start;\n};\n\n' +
    'export struct whitespace_span {\n  uint32 id;\n  uint16 length;\n};\n\n' +
    'export constexpr identifier_span identifier_spans[] ' + idInit + ';\n\n' +
    'export constexpr whitespace_span whitespace_spans[] ' + wsInit + ';';

  FS.writeFileSync(OUT_PATH, out);

})().then(complete, fail);
