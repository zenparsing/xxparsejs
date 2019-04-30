export module Token;

export enum class Token {
  end,
  error,
  comment,
  identifier,
  whitespace,
  template_basic,
  template_head,
  template_middle,
  template_tail,
  string,
  regexp,
  number,

  // Punctuators
  left_brace,               // {
  right_brace,              // }
  left_paren,               // (
  right_paren,              // )
  left_bracket,             // [
  right_bracket,            // ]
  semicolon,                // ;
  colon,                    // :
  comma,                    // ,
  question,                 // ?
  bitwise_and,              // &
  bitwise_and_assign,       // &=
  bitwise_or,               // |
  bitwise_or_assign,        // |=
  bitwise_xor,              // ^
  bitwise_xor_assign,       // ^=
  bitwise_not,              // ~
  bitwise_not_assign,       // ~=
  left_shift,               // <<
  left_shift_assign,        // <<=
  left_shift_zero,          // <<<
  left_shift_zero_assign,   // <<<=
  right_shift,              // >>
  right_shift_assign,       // >>=
  right_shift_zero,         // >>>
  right_shift_zero_assign,  // >>>=
  plus,                     // +
  plus_assign,              // +=
  minus,                    // -
  minus_assign,             // -=
  multiply,                 // *
  multiply_assign,          // *=
  divide,                   // /
  divide_assign,            // /=
  mod,                      // %
  mod_assign,               // %=
  pow,                      // **
  pow_assign,               // **=
  logical_and,              // &&
  logical_or,               // ||
  logical_not,              // !
  less_than,                // <
  less_than_equal,          // <=
  greater_than,             // >
  greater_than_equal,       // >=
  assign,                   // =
  equal,                    // ==
  strict_equal,             // ===
  not_equal,                // !=
  strict_not_equal,         // !==
  increment,                // ++
  decrement,                // --
  dot,                      // .
  dot_3,                    // ...
  fat_arrow,                // =>

  // Keywords
  kw_begin,
  kw_break,
  kw_case,
  kw_catch,
  kw_class,
  kw_const,
  kw_continue,
  kw_debugger,
  kw_default,
  kw_delete,
  kw_do,
  kw_else,
  kw_enum,
  kw_export,
  kw_extends,
  kw_false,
  kw_finally,
  kw_for,
  kw_function,
  kw_if,
  kw_import,
  kw_in,
  kw_instanceof,
  kw_new,
  kw_null,
  kw_return,
  kw_super,
  kw_switch,
  kw_this,
  kw_throw,
  kw_true,
  kw_try,
  kw_typeof,
  kw_var,
  kw_void,
  kw_while,
  kw_with,
  kw_strict_begin,
  kw_implements,
  kw_private,
  kw_public,
  kw_interface,
  kw_let,
  kw_package,
  kw_protected,
  kw_static,
  kw_yield,
  kw_strict_end,
  kw_contextual_begin,
  kw_as,
  kw_async,
  kw_await,
  kw_from,
  kw_of,
  kw_contextual_end,
  kw_end,
};
