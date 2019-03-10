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

void test_hex_numbers() {
  test("Hex numbers - basic", "0xdeadBEAF012345678;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Hex numbers - digit required", "0x;", {
    Token::error,
  });

  test("Hex numbers - invalid lookahead", "0x0q", {
    Token::error
  });
}

void test_binary_numbers() {
  test("Binary numbers - basic", "0b01010;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Binary numbers - digit required", "0b;", {
    Token::error,
  });

  test("Binary numbers - invalid lookahead", "0b0f", {
    Token::error
  });
}

void test_octal_numbers() {
  test("Octal numbers - basic", "0o077;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });

  test("Octal numbers - digit required", "0o;", {
    Token::error,
  });

  test("Octal numbers - invalid lookahead", "0o077a", {
    Token::error
  });
}

void test_line_comments() {
  test("Line comments - basic", ";// abc\n;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Line comments - end of file", "//", {
    Token::comment,
    Token::end,
  });
}

void test_block_comments() {
  test("Block comments - basic", "; /* abc */ ;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Block comments - no nesting", ";/* /* */;", {
    Token::semicolon,
    Token::comment,
    Token::semicolon,
    Token::end,
  });

  test("Block comments - end required", "/*", {
    Token::error,
  });
}

void test_strings() {
  test("Strings - double quote", "\"hello\"", {
    Token::string,
    Token::end,
  });

  test("Strings - double quote", "'hello'", {
    Token::string,
    Token::end,
  });

  test("Strings - unicode escape 1", "'\\uABCD'", {
    Token::string,
    Token::end,
  });

  test("Strings - unicode escape 2", "'\\u{ABCD}'", {
    Token::string,
    Token::end,
  });

  test("Strings - unicode escape out of range", "'\\u{110000}'", {
    Token::error,
  });

  test("Strings - hex escape", "'\\x00BE'", {
    Token::string,
    Token::end,
  });

  test("Strings - invalid hex escape 1", "'\\xZ", {
    Token::error,
  });

  test("Strings - invalid hex escape 1", "'\\xAZ", {
    Token::error,
  });

  test("Strings - legacy octal escapes", "'\\012'", {
    Token::string,
    Token::end,
  });
}

int main() {
  test_hex_numbers();
  test_octal_numbers();
  test_binary_numbers();
  test_line_comments();
  test_block_comments();
  test_strings();
}
