#include <optional>
#include <cassert>

export module Scanner;

import BasicTypes;
import Unicode;
import Token;
import TokenStartTable;
import TokenTrie;

using std::optional;

export bool is_strict_reserved_word(Token t) {
  return
    t > Token::kw_strict_reserved_begin &&
    t < Token::kw_strict_reserved_end;
}

export bool is_contextual_keyword(Token t) {
  return
    t > Token::kw_contextual_begin &&
    t < Token::kw_contextual_end;
}

export Token;

export using SourcePosition = uint32;

export template<typename T>
struct Scanner {

  enum class Context {
    expression,
    template_string,
    div,
  };

  enum class Error {
    none,
    unexpected_character,
    invalid_hex_escape,
    invalid_unicode_escape,
    invalid_identifier_escape,
    unterminated_string,
    unterminated_comment,
    unterminated_template,
    unterminatedregexp,
    missing_exponent,
    invalid_octal_literal,
    invalid_hex_literal,
    invalid_binary_literal,
    invalidnumber_suffix,
    legacy_octal_escape,
    legacy_octal_number,
  };

  struct Result {
    Token token {Token::error};
    SourcePosition start {0};
    SourcePosition end {0};
    bool newline_before {false};
    Error error {Error::none};
    Error strict_error {Error::none};
  };

  Scanner(T& begin, T& end) : _iter {begin}, _end {end} {}

  Token next(Context context = Context::expression) {
    if (_result.token != Token::comment) {
      _result.newline_before = false;
    }

    _result.start = _position;
    _result.error = Error::none;
    _result.strict_error = Error::none;

    while (true) {
      start(context);
      if (_result.token != Token::whitespace) {
        _result.end = _position;
        return _result.token;
      }
    }
  }

  uint32 shift() {
    assert(_iter != _end);
    uint32 cp = *_iter;
    advance();
    return cp;
  }

  void advance() {
    assert(_iter != _end);
    ++_position;
    ++_iter;
  }

  uint32 peek() {
    return _iter == _end ? 0 : *_iter;
  }

  bool peek_range(uint32 low, uint32 high) {
    assert(low != 0 && high != 0);
    auto n = peek();
    return n >= low && n <= high;
  }

  bool can_shift() {
    return _iter != _end;
  }

  void set_token(Token t) {
    _result.token = t;
  }

  void set_error(Error error) {
    _result.error = error;
    _result.token = Token::error;
  }

  void set_strict_error(Error error) {
    _result.strict_error = error;
  }

  void start(Context context) {
    if (!can_shift()) {
      return set_token(Token::end);
    }

    if (auto cp = shift(); cp < 128) {
      switch (token_start_table[cp]) {
        case TokenStartType::punctuator:
          return punctuator(cp);

        case TokenStartType::whitespace:
          return set_token(Token::whitespace);

        case TokenStartType::newline:
          return newline(cp);

        case TokenStartType::string:
          return string(cp);

        case TokenStartType::identifier:
          return identifier(cp);

        case TokenStartType::dot:
          return peek_range('0', '9')
            ? number(cp)
            : punctuator(cp);

        case TokenStartType::slash:
          if (auto next = peek(); next == '/') {
            return line_comment();
          } else if (next == '*') {
            return block_comment();
          } else if (context == Context::div) {
            return punctuator(cp);
          }
          return regexp();

        case TokenStartType::zero:
          if (auto next = peek(); next == 'X' || next == 'x') {
            return hex_number();
          } else if (next == 'B' || next == 'b') {
            return binary_number();
          } else if (next == 'O' || next == 'o') {
            return octal_number();
          } else if (next >= '0' && next <= '7') {
            return legacy_octal_number();
          }
          return number(cp);

        case TokenStartType::digit:
          return number(cp);

        case TokenStartType::backtick:
          return template_string(cp);

        case TokenStartType::right_brace:
          return context == Context::template_string
            ? template_string(cp)
            : punctuator(cp);
      }
    } else if (is_newline_char(cp)) {
      return newline(cp);
    } else if (is_whitespace(cp)) {
      return set_token(Token::whitespace);
    } else if (is_identifier_start(cp)) {
      return identifier(cp);
    }

    set_error(Error::unexpected_character);
  }

  void punctuator(uint32 cp) {
    set_token(TokenTrie<Scanner>::match_punctuator(*this, cp));
  }

  void template_string(uint32 cp) {
    while (can_shift()) {
      if (auto n = shift(); n == '`') {
        return set_token(cp == '`'
          ? Token::template_basic
          : Token::template_tail
        );
      } else if (n == '$' && peek() == '{') {
        advance();
        return set_token(cp == '`'
          ? Token::template_head
          : Token::template_middle
        );
      } else if (n == '\\') {
        string_escape(false);
      }
    }
    set_error(Error::unterminated_template);
  }

  void newline(uint32 cp) {
    set_token(Token::whitespace);
    if (cp == '\r' && peek() == '\n') {
      advance();
    }
    _result.newline_before = true;
  }

  void identifier(uint32 cp) {
    set_token(TokenTrie<Scanner>::match_keyword(*this, cp));
    while (true) {
      if (auto n = peek(); is_identifier_part(n)) {
        set_token(Token::identifier);
        advance();
      } else if (n == '\\') {
        set_token(Token::identifier);
        advance();
        if (peek() != 'u') {
          return set_error(Error::invalid_identifier_escape);
        }
        advance();
        if (!unicode_escape_sequence()) {
          return set_error(Error::invalid_identifier_escape);
        }
      } else {
        break;
      }
    }
  }

  void number(uint32 cp) {
    set_token(Token::number);
    // TODO: set double parser state to whole
    if (cp == '.') {
      // TODO: set double parser state to fraction
      maybe_decimal_integer();
    } else {
      decimal_integer(cp);
      if (peek() == '.') {
        advance();
        // TODO: set double parser state to fraction
        maybe_decimal_integer();
      }
    }

    if (auto n = peek(); n == 'e' || n == 'E') {
      advance();
      // TODO: set double parser state to exponent
      if (auto n = peek(); n == '-') {
        advance();
        // TODO: set double parser state to negative exponent
      } else if (n == '+') {
        advance();
      }
      if (!maybe_decimal_integer()) {
        return set_error(Error::missing_exponent);
      }
    }

    number_suffix();
  }

  bool maybe_decimal_integer() {
    if (peek_range('0', '9')) {
      decimal_integer(shift());
      return true;
    }
    return false;
  }

  void decimal_integer(uint32 cp) {
    assert(cp >= '0' && cp <= '9');
    int val = cp - '0';
    // TODO: send value to double parser
    while (peek_range('0', '9')) {
      // TODO: send value to double parser
      val = shift() - '0';
    }
  }

  void legacy_octal_number() {
    set_strict_error(Error::legacy_octal_number);
    octal_integer();
  }

  void octal_number() {
    assert(peek() == 'o');
    advance();
    octal_integer();
  }

  void octal_integer() {
    if (!peek_range('0', '7')) {
      return set_error(Error::invalid_octal_literal);
    }
    set_token(Token::number);
    advance();
    while (true) {
      if (peek_range('0', '7')) {
        advance();
      } else {
        break;
      }
    }
    number_suffix();
  }

  void hex_number() {
    assert(peek() == 'x');
    advance();
    if (!hex_char_value(peek())) {
      return set_error(Error::invalid_hex_literal);
    }
    set_token(Token::number);
    advance();
    while (hex_char_value(peek())) {
      advance();
    }
    number_suffix();
  }

  void binary_number() {
    assert(peek() == 'b');
    advance();
    if (!peek_range('0', '1')) {
      return set_error(Error::invalid_binary_literal);
    }
    set_token(Token::number);
    advance();
    while (true) {
      if (peek_range('0', '1')) {
        advance();
      } else {
        break;
      }
    }
    number_suffix();
  }

  void number_suffix() {
    // TODO: we may need to match for unicode escape sequences as well
    if (auto n = peek(); n < 128) {

      if (token_start_table[n] == TokenStartType::identifier) {
        set_error(Error::invalidnumber_suffix);
      }
    } else if (is_identifier_start(n)) {
      set_error(Error::invalidnumber_suffix);
    }
  }

  void regexp() {
    set_token(Token::regexp);

    bool backslash = false;
    bool in_class = false;

    while (can_shift()) {
      if (auto n = shift(); is_newline_char(n)) {
        break;
      } else if (backslash) {
        backslash = false;
      } else if (n == '[') {
        in_class = true;
      } else if (n == ']' && in_class) {
        in_class = false;
      } else if (n == '\\') {
        backslash = true;
      } else if (n == '/' && !in_class) {
        // TODO: this could be a unicode escape sequence as well
        regexp_flags();
        return;
      }
    }

    set_error(Error::unterminatedregexp);
  }

  void regexp_flags() {
    // TODO: this could be a unicode escape sequence as well
    // TODO: validate flags here?
    while (is_identifier_part(peek())) {
      advance();
    }
  }

  void line_comment() {
    assert(peek() == '/');
    advance();
    set_token(Token::comment);
    while (can_shift() && !is_newline_char(peek())) {
      advance();
    }
  }

  void block_comment() {
    assert(peek() == '*');
    advance();
    set_token(Token::comment);
    while (can_shift()) {
      if (auto cp = shift(); is_newline_char(cp)) {
        if (cp == '\r' && peek() == '\n') {
          advance();
        }
        _result.newline_before = true;
      } else if (cp == '*' && peek() == '/') {
        advance();
        return;
      }
    }
    set_error(Error::unterminated_comment);
  }

  void string(uint32 delim) {
    set_token(Token::string);
    while (can_shift()) {
      if (auto n = shift(); n == delim) {
        return;
      } else if (n == '\\') {
        string_escape(true);
      } else if (n == '\r' || n == '\n') {
        break;
      }
    }
    set_error(Error::unterminated_string);
  }

  optional<uint32> string_escape(bool allow_legacy_octal = false) {
    if (!can_shift()) {
      return {};
    }

    switch (uint32 cp = shift()) {
      case 't': return '\t';
      case 'b': return '\b';
      case 'v': return '\v';
      case 'f': return '\f';
      case 'r': return '\r';
      case 'n': return '\n';

      case '\r':
        if (peek() == '\n') {
          advance();
        }
        return {};

      case '\n':
      case 0x2028:
      case 0x2029:
        return {};

      case '0':
        if (allow_legacy_octal && peek_range('0', '7')) {
          return string_escape_octal(cp, 2);
        }
        return {0};

      case '1':
      case '2':
      case '3':
        return allow_legacy_octal ? string_escape_octal(cp, 2) : cp;

      case '4':
      case '5':
      case '6':
      case '7':
        return allow_legacy_octal ? string_escape_octal(cp, 1) : cp;

      case 'x':
        if (auto v = string_escape_hex(2)) {
          return v;
        }
        set_error(Error::invalid_hex_escape);
        return {};

      case 'u':
        return unicode_escape_sequence();

      default:
        return cp;

    }
  }

  optional<uint32> unicode_escape_sequence() {
    if (peek() == '{') {
      advance();
      if (auto v = string_escape_hex(1, 6); v && peek() == '}') {
        advance();
        return v;
      }
    } else {
      if (auto v = string_escape_hex(4)) {
        return v;
      }
    }
    set_error(Error::invalid_unicode_escape);
    return {};
  }

  uint32 string_escape_octal(uint32 first, int max) {
    assert(first >= '0' && first <= '7');
    set_strict_error(Error::legacy_octal_escape);
    uint32 val = first - '0';
    for (int count = 0; count < max; ++count) {
      if (auto n = peek(); n >= '0' && n <= '7') {
        advance();
        val = val * 8 + n - '0';
      } else {
        break;
      }
    }
    return val;
  }

  auto string_escape_hex(int length) {
    return string_escape_hex(length, length);
  }

  optional<uint32> string_escape_hex(int min, int max) {
    uint32 val = 0;
    int count = 0;
    for (; count < max; ++count) {
      if (auto v = hex_char_value(peek())) {
        advance();
        val = val * 16 + *v;
      } else {
        break;
      }
    }
    if (count >= min && val <= 0x10ffff) {
      return val;
    }
    return {};
  }

  static optional<uint32> hex_char_value(uint32 cp) {
    if (cp >= '0' && cp <= '9') {
      return cp - 0;
    }
    if (cp >= 'A' && cp <= 'F') {
      return cp - 'A';
    }
    if (cp >= 'a' && cp <= 'f') {
      return cp - 'a';
    }
    return {};
  }

  static bool is_newline_char(uint32 cp) {
    switch (cp) {
      case '\n':
      case '\r':
      case 0x2028:
      case 0x2029:
        return true;
    }
    return false;
  }

  T _iter;
  T _end;
  SourcePosition _position {0};
  Result _result;

};
