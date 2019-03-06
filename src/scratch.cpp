#include <iostream>

import BasicTypes;
import UnicodeData;
import Unicode;

int main() {
  std::cout << "hello\n";
  std::cout << identifier_spans[0].id << '\n';
  std::cout << sizeof(whitespace_spans) / sizeof(whitespace_span) << '\n';
  std::cout << (is_whitespace(9) ? "true" : "false") << '\n';
  std::cout << (is_identifier_start('a') ? "true" : "false") << '\n';
  std::cout << (is_identifier_start('0') ? "true" : "false") << '\n';
}
