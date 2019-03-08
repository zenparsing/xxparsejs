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
    if (t == Token::end) {
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

int main() {
  test("Hex numbers", "0xdeadBEAF012345678;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });
}
