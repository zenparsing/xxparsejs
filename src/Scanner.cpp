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
    unterminated_regexp,
    missing_exponent,
    invalid_octal_literal,
    invalid_hex_literal,
    invalid_binary_literal,
    invalid_number_suffix,
    legacy_octal_escape,
    legacy_octal_number,
  };

  struct Result {
    Token token {Token::error};
    SourcePosition start {0};
    SourcePosition end {0};
    bool newline_before {false};
    double number_value {0};
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
      _start(context);
      if (_result.token != Token::whitespace) {
        _result.end = _position;
        return _result.token;
      }
    }
  }

  uint32 _shift() {
    assert(_iter != _end);
    uint32 cp = *_iter;
    _advance();
    return cp;
  }

  void _advance() {
    assert(_iter != _end);
    ++_position;
    ++_iter;
  }

  uint32 _peek() {
    return _iter == _end ? 0 : *_iter;
  }

  bool _peek_range(uint32 low, uint32 high) {
    auto n = _peek();
    return n >= low && n <= high;
  }

  bool _can_shift() {
    return _iter != _end;
  }

  void _set_error(Error error) {
    _result.error = error;
    _result.token = Token::error;
  }

  void _set_strict_error(Error error) {
    _result.strict_error = error;
  }

  void _set_token(Token t) {
    _result.token = t;
  }

  void _start(Context context) {
    if (!_can_shift()) {
      return _set_token(Token::end);
    }

    if (auto cp = _shift(); cp < 128) {
      switch (token_start_table[cp]) {
        case TokenStartType::punctuator:
          return _punctuator(cp);

        case TokenStartType::whitespace:
          return _set_token(Token::whitespace);

        case TokenStartType::newline:
          return _newline(cp);

        case TokenStartType::string:
          return _string(cp);

        case TokenStartType::identifier:
          return _identifier(cp);

        case TokenStartType::dot:
          return _peek_range('0', '9')
            ? _number(cp)
            : _punctuator(cp);

        case TokenStartType::slash:
          if (auto next = _peek(); next == '/') {
            return _line_comment();
          } else if (next == '*') {
            return _block_comment();
          } else if (context == Context::div) {
            return _punctuator(cp);
          }
          return _regexp();

        case TokenStartType::zero:
          if (auto next = _peek(); next == 'X' || next == 'x') {
            return _hex_number();
          } else if (next == 'B' || next == 'b') {
            return _binary_number();
          } else if (next == 'O' || next == 'o') {
            return _octal_number();
          } else if (next >= '0' && next <= '7') {
            return _legacy_octal_number();
          }
          return _number(cp);

        case TokenStartType::digit:
          return _number(cp);

        case TokenStartType::backtick:
          return _template(cp);

        case TokenStartType::right_brace:
          return context == Context::template_string
            ? _template(cp)
            : _punctuator(cp);
      }
    } else if (is_newline_char(cp)) {
      return _newline(cp);
    } else if (is_whitespace(cp)) {
      return _set_token(Token::whitespace);
    } else if (is_identifier_start(cp)) {
      return _identifier(cp);
    }

    _set_error(Error::unexpected_character);
  }

  void _punctuator(uint32 cp) {
    _set_token(TokenTrie<Scanner>::match_punctuator(*this, cp));
  }

  void _template(uint32 cp) {
    while (_can_shift()) {
      if (auto n = _shift(); n == '`') {
        return _set_token(cp == '`'
          ? Token::template_basic
          : Token::template_end
        );
      } else if (n == '$' && _peek() == '{') {
        _advance();
        return _set_token(cp == '`'
          ? Token::template_start
          : Token::template_middle
        );
      } else if (n == '\\') {
        // TODO: template strings can still parse even if escapes
        // are not valid
        _string_escape(false);
      }
    }
    _set_error(Error::unterminated_template);
  }

  void _newline(uint32 cp) {
    _set_token(Token::whitespace);
    if (cp == '\r' && _peek() == '\n') {
      _advance();
    }
    _result.newline_before = true;
  }

  void _identifier(uint32 cp) {
    _set_token(TokenTrie<Scanner>::match_keyword(*this, cp));
    while (true) {
      if (auto n = _peek(); is_identifier_part(n)) {
        _set_token(Token::identifier);
        _advance();
      } else if (n == '\\') {
        _set_token(Token::identifier);
        _advance();
        if (_peek() != 'u') {
          return _set_error(Error::invalid_identifier_escape);
        }
        _advance();
        if (!_unicode_escape_sequence()) {
          return _set_error(Error::invalid_identifier_escape);
        }
      } else {
        break;
      }
    }
  }

  void _number(uint32 cp) {
    _set_token(Token::number);
    // TODO: set double parser state to whole
    if (cp == '.') {
      // TODO: set double parser state to fraction
      _maybe_decimal_integer();
    } else {
      _decimal_integer(cp);
      if (_peek() == '.') {
        _advance();
        // TODO: set double parser state to fraction
        _maybe_decimal_integer();
      }
    }

    if (auto n = _peek(); n == 'e' || n == 'E') {
      _advance();
      // TODO: set double parser state to exponent
      if (auto n = _peek(); n == '-') {
        _advance();
        // TODO: set double parser state to negative exponent
      } else if (n == '+') {
        _advance();
      }
      if (!_maybe_decimal_integer()) {
        return _set_error(Error::missing_exponent);
      }
    }

    _number_suffix();
  }

  bool _maybe_decimal_integer() {
    if (_peek_range('0', '9')) {
      _decimal_integer(_shift());
      return true;
    }
    return false;
  }

  void _decimal_integer(uint32 cp) {
    assert(cp >= '0' && cp <= '9');
    int val = cp - '0';
    // TODO: send value to double parser
    while (_peek_range('0', '9')) {
      // TODO: send value to double parser
      val = _shift() - '0';
    }
  }

  void _legacy_octal_number() {
    _set_strict_error(Error::legacy_octal_number);
    _octal_integer();
  }

  void _octal_number() {
    assert(_peek() == 'o');
    _advance();
    _octal_integer();
  }

  void _octal_integer() {
    if (!_peek_range('0', '7')) {
      return _set_error(Error::invalid_octal_literal);
    }
    _set_token(Token::number);
    _advance();
    while (true) {
      if (_peek_range('0', '7')) {
        _advance();
      } else {
        break;
      }
    }
    _number_suffix();
  }

  void _hex_number() {
    assert(_peek() == 'x');
    _advance();
    if (!hex_char_value(_peek())) {
      return _set_error(Error::invalid_hex_literal);
    }
    _set_token(Token::number);
    _advance();
    while (hex_char_value(_peek())) {
      _advance();
    }
    _number_suffix();
  }

  void _binary_number() {
    assert(_peek() == 'b');
    _advance();
    if (!_peek_range('0', '1')) {
      return _set_error(Error::invalid_binary_literal);
    }
    _set_token(Token::number);
    _advance();
    while (true) {
      if (_peek_range('0', '1')) {
        _advance();
      } else {
        break;
      }
    }
    _number_suffix();
  }

  void _number_suffix() {
    // TODO: we may need to match for unicode escape sequences as well
    if (auto n = _peek(); n < 128) {
      if (token_start_table[n] == TokenStartType::identifier) {
        _set_error(Error::invalid_number_suffix);
      }
    } else if (is_identifier_start(n)) {
      _set_error(Error::invalid_number_suffix);
    }
  }

  void _regexp() {
    _set_token(Token::regexp);

    bool backslash = false;
    bool in_class = false;

    while (_can_shift()) {
      if (auto n = _shift(); is_newline_char(n)) {
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
        _regexp_flags();
        return;
      }
    }

    _set_error(Error::unterminated_regexp);
  }

  void _regexp_flags() {
    // TODO: this could be a unicode escape sequence as well
    // TODO: validate flags here?
    while (is_identifier_part(_peek())) {
      _advance();
    }
  }

  void _line_comment() {
    assert(_peek() == '/');
    _advance();
    _set_token(Token::comment);
    while (_can_shift() && !is_newline_char(_peek())) {
      _advance();
    }
  }

  void _block_comment() {
    assert(_peek() == '*');
    _advance();
    _set_token(Token::comment);
    while (_can_shift()) {
      if (auto cp = _shift(); is_newline_char(cp)) {
        if (cp == '\r' && _peek() == '\n') {
          _advance();
        }
        _result.newline_before = true;
      } else if (cp == '*' && _peek() == '/') {
        _advance();
        return;
      }
    }
    _set_error(Error::unterminated_comment);
  }

  void _string(uint32 delim) {
    _set_token(Token::string);
    while (_can_shift()) {
      if (auto n = _shift(); n == delim) {
        return;
      } else if (n == '\\') {
        _string_escape(true);
      } else if (n == '\r' || n == '\n') {
        break;
      }
    }
    _set_error(Error::unterminated_string);
  }

  optional<uint32> _string_escape(bool allow_legacy_octal = false) {
    if (!_can_shift()) {
      return {};
    }

    switch (uint32 cp = _shift()) {
      case 't': return '\t';
      case 'b': return '\b';
      case 'v': return '\v';
      case 'f': return '\f';
      case 'r': return '\r';
      case 'n': return '\n';

      case '\r':
        if (_peek() == '\n') {
          _advance();
        }
        return {};

      case '\n':
      case 0x2028:
      case 0x2029:
        return {};

      case '0':
        if (allow_legacy_octal && _peek_range('0', '7')) {
          return _string_escape_octal(cp, 2);
        }
        return {0};

      case '1':
      case '2':
      case '3':
        return allow_legacy_octal ? _string_escape_octal(cp, 2) : cp;

      case '4':
      case '5':
      case '6':
      case '7':
        return allow_legacy_octal ? _string_escape_octal(cp, 1) : cp;

      case 'x':
        if (auto v = _string_escape_hex(2)) {
          return v;
        }
        _set_error(Error::invalid_hex_escape);
        return {};

      case 'u':
        return _unicode_escape_sequence();

      default:
        return cp;

    }
  }

  optional<uint32> _unicode_escape_sequence() {
    if (_peek() == '{') {
      _advance();
      if (auto v = _string_escape_hex(1, 6); v && _peek() == '}') {
        _advance();
        return v;
      }
    } else {
      if (auto v = _string_escape_hex(4)) {
        return v;
      }
    }
    _set_error(Error::invalid_unicode_escape);
    return {};
  }

  uint32 _string_escape_octal(uint32 first, int max) {
    assert(first >= '0' && first <= '7');
    _set_strict_error(Error::legacy_octal_escape);
    uint32 val = first - '0';
    for (int count = 0; count < max; ++count) {
      if (auto n = _peek(); n >= '0' && n <= '7') {
        _advance();
        val = val * 8 + n - '0';
      } else {
        break;
      }
    }
    return val;
  }

  auto _string_escape_hex(int length) {
    return _string_escape_hex(length, length);
  }

  optional<uint32> _string_escape_hex(int min, int max) {
    uint32 val = 0;
    int count = 0;
    for (; count < max; ++count) {
      if (auto v = hex_char_value(_peek())) {
        _advance();
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

  static bool is_newline_char(uint32 cp, bool ascii_only = false) {
    if (cp == '\n' || cp == '\r') {
      return true;
    }
    if (ascii_only) {
      return false;
    }
    return cp == 0x2028 || cp == 0x2029;
  }

  T _iter;
  T _end;
  SourcePosition _position {0};
  Result _result;

};
