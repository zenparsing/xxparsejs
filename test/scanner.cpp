#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

import Scanner;
import test.TokenStrings;

using std::string;
using std::vector;

void test(
  const string& test_name,
  const string& input,
  const vector<Token>& expected
) {
  Scanner scanner {input.begin(), input.end()};
  vector<Token> actual;
  while (true) {
    Token t = scanner.next();
    actual.push_back(t);
    if (t == Token::end || t == Token::error) {
      break;
    }
  }
  if (!std::equal(actual.begin(), actual.end(), expected.begin())) {
    std::cerr
      << "[" << test_name << "]\n"
      << "Error: Token streams are not equal\n"
      << "Input string: " << input << "\n"
      << "Output tokens:\n";

    for (auto t : actual) {
      std::cerr << "- " << t << "\n";
    }
    std::exit(1);
  }
}

void test_number() {
  test("Number - integer", "1234", {
    Token::number,
    Token::end,
  });

  test("Number - with decimal point", "234.45", {
    Token::number,
    Token::end,
  });

  test("Number - with exponent", "234.45e12", {
    Token::number,
    Token::end,
  });

  test("Number - with exponent sign +", "234e+12", {
    Token::number,
    Token::end,
  });

  test("Number - with exponent sign -", "234e-12", {
    Token::number,
    Token::end,
  });

  test("Number - leading decimal point", ".234", {
    Token::number,
    Token::end,
  });

  test("Number - trailing decimal point", "234.;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });
}

void test_hex_number() {
  test("Hex number - basic", "0xdeadBEAF012345678;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Hex number - digit required", "0x;", {
    Token::error,
  });

  test("Hex number - invalid lookahead", "0x0q", {
    Token::error
  });
}

void test_binary_number() {
  test("Binary number - basic", "0b01010;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Binary number - digit required", "0b;", {
    Token::error,
  });

  test("Binary number - invalid lookahead", "0b0f", {
    Token::error
  });
}

void test_octal_number() {
  test("Octal number - basic", "0o077;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Octal number - digit required", "0o;", {
    Token::error,
  });

  test("Octal number - invalid lookahead", "0o077a", {
    Token::error
  });
}

void test_line_comment() {
  test("Line comment - basic", ";// abc\n;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Line comment - end of file", "//", {
    Token::comment,
    Token::end,
  });
}

void test_block_comment() {
  test("Block comment - basic", "; /* abc */ ;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Block comment - no nesting", ";/* /* */;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Block comment - end required", "/*", {
    Token::error,
  });
}

void test_string() {
  test("String - double quote", "\"hello\"", {
    Token::string,
    Token::end,
  });

  test("String - double quote", "'hello'", {
    Token::string,
    Token::end,
  });

  test("String - unicode escape 1", "'\\uABCD'", {
    Token::string,
    Token::end,
  });

  test("String - unicode escape 2", "'\\u{ABCD}'", {
    Token::string,
    Token::end,
  });

  test("String - unicode escape out of range", "'\\u{110000}'", {
    Token::error,
  });

  test("String - hex escape", "'\\x00BE'", {
    Token::string,
    Token::end,
  });

  test("String - invalid hex escape 1", "'\\xZ", {
    Token::error,
  });

  test("String - invalid hex escape 1", "'\\xAZ", {
    Token::error,
  });

  test("String - legacy octal escapes", "'\\012'", {
    Token::string,
    Token::end,
  });
}

void test_identifier() {
  test("Identifier - max munch", "iffy;", {
    Token::identifier,
    Token::semicolon,
    Token::end,
  });

  test("Identifier - unicode escape", "a\\u{62}c;", {
    Token::identifier,
    Token::semicolon,
    Token::end,
  });
}

int main() {
  test_number();
  test_hex_number();
  test_octal_number();
  test_binary_number();
  test_line_comment();
  test_block_comment();
  test_string();
  test_identifier();
}
