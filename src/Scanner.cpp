#include <optional>

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
    invalid_hex_escape,
    invalid_unicode_escape,
    unterminated_string,
    unterminated_comment,
    unterminated_template,
    invalid_number_literal,
  };

  struct Result {
    Token token {Token::error};
    SourcePosition start {0};
    SourcePosition end {0};
    bool newline_before {false};
    double number_value {0};
    Error error {Error::none};
  };

  Scanner(T& begin, T& end) : _iter {begin}, _end {end} {}

  Token next(Context context = Context::expression) {
    if (_result.token != Token::comment) {
      _result.newline_before = false;
    }

    _result.start = _position;
    _result.error = Error::none;

    while (true) {
      _result.token = _start(context);
      if (_result.error != Error::none) {
        _result.token = Token::error;
      }
      if (_result.token != Token::whitespace) {
        _result.end = _position;
        return _result.token;
      }
    }
  }

  uint32 _shift() {
    // assert(_iter != _end)
    uint32 cp = *_iter;
    _advance();
    return cp;
  }

  void _advance() {
    // assert(_iter != _end)
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
  }

  Token _start(Context context) {
    if (!_can_shift()) {
      return Token::end;
    }

    if (auto cp = _shift(); cp < 128) {
      switch (token_start_table[cp]) {
        case TokenStartType::punctuator:
          return _punctuator(cp);

        case TokenStartType::whitespace:
          return Token::whitespace;

        case TokenStartType::newline:
          return _newline(cp);

        case TokenStartType::string:
          return _string(cp);

        case TokenStartType::identifier:
          return _identifier(cp);

        case TokenStartType::dot:
          return is_ascii_digit(_peek())
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

        default:
          return Token::error;
      }
    } else if (is_newline_char(cp)) {
      return _newline(cp);
    } else if (is_whitespace(cp)) {
      return Token::whitespace;
    } else if (is_identifier_start(cp)) {
      return _identifier(cp);
    }

    return Token::error;
  }

  Token _punctuator(uint32 cp) {
    return TokenTrie<Scanner>::match_punctuator(*this, cp);
  }

  Token _template(uint32 cp) {
    while (_can_shift()) {
      if (auto n = _shift(); n == '`') {
        return cp == '`'
          ? Token::template_basic
          : Token::template_end;
      } else if (n == '$' && _peek() == '{') {
        _advance();
        return cp == '`'
          ? Token::template_start
          : Token::template_middle;
      } else if (n == '\\') {
        _string_escape(false);
      }
    }
    return Token::error;
  }

  Token _newline(uint32 cp) {
    if (cp == '\r' && _peek() == '\n') {
      _advance();
    }
    _result.newline_before = true;
    return Token::whitespace;
  }

  Token _identifier(uint32 cp) {
    Token t = TokenTrie<Scanner>::match_keyword(*this, cp);
    if (t == Token::error) {

    }
    return t;
  }

  Token _number(uint32 cp) {
    return Token::error;
  }

  Token _legacy_octal_number() {
    // TODO: record strict mode error
    return _read_octal();
  }

  Token _octal_number() {
    // assert(_peek() == 'o')
    _advance();
    return _read_octal();
  }

  Token _read_octal() {
    // TODO: store double value
    if (!_peek_range('0', '7')) {
      return Token::error;
    }
    _advance();
    while (true) {
      if (_peek_range('0', '7')) {
        _advance();
      } else {
        break;
      }
    }
    _number_suffix();
    return Token::number;
  }

  Token _hex_number() {
    // TODO: store number value
    // assert(_peek() == 'x')
    _advance();
    if (!hex_char_value(_peek())) {
      return Token::error;
    }
    _advance();
    while (hex_char_value(_peek())) {
      _advance();
    }
    _number_suffix();
    return Token::number;
  }

  Token _binary_number() {
    // TODO: store number value
    // assert(_peek() == 'b')
    _advance();
    if (!_peek_range('0', '1')) {
      return Token::error;
    }
    _advance();
    while (true) {
      if (_peek_range('0', '1')) {
        _advance();
      } else {
        break;
      }
    }
    _number_suffix();
    return Token::number;
  }

  void _number_suffix() {
    if (auto n = _peek(); n < 128) {
      if (token_start_table[n] == TokenStartType::identifier) {
        _set_error(Error::invalid_number_literal);
      }
    } else if (is_identifier_start(n)) {
      _set_error(Error::invalid_number_literal);
    }
  }

  Token _regexp() {
    return Token::error;
  }

  Token _line_comment() {
    // assert(_peek() == '/')
    _advance();
    while (_can_shift() && !is_newline_char(_peek())) {
      _advance();
    }
    return Token::comment;
  }

  Token _block_comment() {
    // assert(_peek() == '*')
    _advance();
    while (true) {
      if (!_can_shift()) {
        _set_error(Error::unterminated_comment);
        break;
      }
      if (auto cp = _shift(); is_newline_char(cp)) {
        if (cp == '\r' && _peek() == '\n') {
          _advance();
        }
        _result.newline_before = true;
      } else if (cp == '*' && _peek() == '/') {
        _advance();
        break;
      }
    }
    return Token::comment;
  }

  Token _string(uint32 delim) {
    while (_can_shift()) {
      if (auto n = _shift(); n == delim) {
        return Token::string;
      } else if (n == '\\') {
        _string_escape(true);
      } else if (n == '\r' || n == '\n') {
        break;
      }
    }
    _set_error(Error::unterminated_string);
    return Token::string;
  }

  optional<uint32> _string_escape(bool allow_legacy_octal = false) {
    if (!_can_shift()) {
      _set_error(Error::unterminated_string);
      return {};
    }

    uint32 cp = _shift();

    switch (cp) {
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
        if (_peek_range('0', '7')) {
          return _string_escape_octal(cp, 2);
        }
        return {0};

      case '1':
      case '2':
      case '3':
        return _string_escape_octal(cp, 2);

      case '4':
      case '5':
      case '6':
      case '7':
        return _string_escape_octal(cp, 1);

      case 'x':
        if (auto v = _string_escape_hex(2)) {
          return v;
        }
        _set_error(Error::invalid_hex_escape);
        return {};

      case 'u':
        if (_peek() == '{') {
          _advance();
          if (auto v = _string_escape_hex(1, 6)) {
            if (_peek() == '}') {
              _advance();
              return v;
            }
          }
        } else {
          if (auto v = _string_escape_hex(4)) {
            return v;
          }
        }
        _set_error(Error::invalid_unicode_escape);
        return {};

      default:
        return cp;

    }
  }

  uint32 _string_escape_octal(uint32 first, int max) {
    // TODO: record strict mode error
    // assert(first >= '0' && first <= '7')
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

  static bool is_ascii_digit(uint32 cp) {
    return cp >= '0' && cp <= '9';
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
