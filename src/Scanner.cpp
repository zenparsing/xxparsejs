export module Scanner;

import BasicTypes;
import Unicode;
import Token;
import TokenStartTable;
import TokenTrie;

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

// TODO: why does this have to be exported?
export bool is_ascii_digit(uint32 cp) {
  return cp >= '0' && cp <= '9';
}

// TODO: why does this have to be exported?
export bool is_hex_digit(uint32 cp) {
  return
    cp >= '0' && cp <= '9' ||
    cp >= 'a' && cp <= 'f' ||
    cp >= 'A' && cp <= 'F';
}

// TODO: why does this have to be exported?
export bool is_newline_char(uint32 cp, bool ascii_only = false) {
  if (cp == '\n' || cp == '\r') {
    return true;
  }

  if (ascii_only) {
    return false;
  }

  return cp == 0x2028 || cp == 0x2029;
}

export Token;

export using SourcePosition = uint32;

export struct TokenSpan {
  Token token {Token::error};
  SourcePosition start {0};
  SourcePosition end {0};
  bool newline_before {false};
};

export template<typename T>
struct Scanner {

  enum class Context {
    expression,
    template_string,
    div,
  };

  Scanner(T& begin, T& end) : _iter {begin}, _end {end} {}

  Token next(Context context = Context::expression) {
    if (_span.token != Token::comment) {
      _span.newline_before = false;
    }

    _span.start = _position;

    while (true) {
      _span.token = _start(context);
      if (_span.token != Token::whitespace) {
        _span.end = _position;
        return _span.token;
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
    return Token::error;
  }

  Token _newline(uint32 cp) {
    if (cp == '\r' && _peek() == '\n') {
      _advance();
    }
    _span.newline_before = true;
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
    _read_integer_suffix();
    return _finish_number();
  }

  Token _hex_number() {
    // assert(_peek() == 'x')
    _advance();
    if (!is_hex_digit(_peek())) {
      return Token::error;
    }
    _advance();
    while (is_hex_digit(_peek())) {
      _advance();
    }
    _read_integer_suffix();
    return _finish_number();
  }

  Token _binary_number() {
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
    _read_integer_suffix();
    return _finish_number();
  }

  void _read_integer_suffix() {
    if (_peek() == 'n') {
      // TODO: record this info in span
      _advance();
    }
  }

  Token _finish_number() {
    if (auto n = _peek(); n < 128) {
      if (token_start_table[n] == TokenStartType::identifier) {
        return Token::error;
      }
    } else if (is_identifier_start(n)) {
      return Token::error;
    }
    return Token::number;
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
    while (_can_shift()) {
      if (_shift() == '*' && _peek() == '/') {
        _advance();
        return Token::comment;
      }
    }
    return Token::error;
  }

  Token _string(uint32 cp) {
    return Token::error;
  }

  T _iter;
  T _end;
  SourcePosition _position {0};
  TokenSpan _span;

};
