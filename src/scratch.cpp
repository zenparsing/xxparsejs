#include <iostream>
#include <vector>

import BasicTypes;
import UnicodeData;
import Unicode;
import Scanner;

int main() {
  std::cout << "hello\n";
  std::cout << identifier_spans[0].id << '\n';
  std::cout << sizeof(whitespace_spans) / sizeof(WhitespaceSpan) << '\n';
  std::cout << (is_whitespace(9) ? "true" : "false") << '\n';
  std::cout << (is_identifier_start('a') ? "true" : "false") << '\n';
  std::cout << (is_identifier_start('0') ? "true" : "false") << '\n';

  std::vector<uint32> vec {' ', '.', 'i', 'f', ';', 'e', 'v'};
  Scanner scanner {vec.begin(), vec.end()};
  std::cout << "Scanning " << static_cast<int>(scanner.next()) << "\n";
  std::cout << static_cast<int>(scanner.next()) << "\n";
}
