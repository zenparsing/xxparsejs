export module Unicode;

import BasicTypes;
import UnicodeData;

struct whitespace_data {
  static constexpr int count = sizeof(whitespace_spans) / sizeof(whitespace_span);
  static constexpr auto table = whitespace_spans;
  using element = whitespace_span;
};

struct identifier_data {
  static constexpr int count = sizeof(identifier_spans) / sizeof(identifier_span);
  static constexpr auto table = identifier_spans;
  using element = identifier_span;
};

template<typename T>
const typename T::element* search_table(uint32 code) {
  int right = T::count - 1;
  int left = 0;

  while (left <= right) {
    int mid = (left + right) >> 1;
    const T::element& span = T::table[mid];

    if (code < span.id) {
      right = mid - 1;
    } else if (code == span.id || code <= span.id + span.length) {
      return &span;
    } else {
      left = mid + 1;
    }
  }

  return nullptr;
}

export bool is_whitespace(uint32 code) {
  return search_table<whitespace_data>(code) != nullptr;
}

export bool is_identifier_start(uint32 code) {
  auto* span = search_table<identifier_data>(code);
  return span != nullptr && span->start;
}

export bool is_identifier_continue(uint32 code) {
  return search_table<identifier_data>(code) != nullptr;
}
