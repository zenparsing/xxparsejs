#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

import Scanner;

template<typename T>
void print_tokens(const T& tokens) {
  for (auto t : tokens) {
    std::cout << static_cast<int>(t) << "\n";
  }
}

void test(const std::string& input, const std::vector<Token>& expected) {
  Scanner scanner {input.begin(), input.end()};
  std::vector<Token> actual;
  while (true) {
    Token t = scanner.next();
    actual.push_back(t);
    if (t == Token::end) {
      break;
    }
  }
  if (!std::equal(actual.begin(), actual.end(), expected.begin())) {
    std::cout << "Not equal" << "\n";
    print_tokens(actual);
    std::exit(1);
  }
}

int main() {
  test("0xdeadBEAF012345678;", {
    Token::number,
    Token::semicolon,
    Token::end,
  });
}
