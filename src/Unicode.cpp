export module Unicode;

import BasicTypes;
import UnicodeData;

struct WhitespaceData {
  static constexpr int count = sizeof(whitespace_spans) / sizeof(WhitespaceSpan);
  static constexpr auto table = whitespace_spans;
  using Element = WhitespaceSpan;
};

struct IdentifierData {
  static constexpr int count = sizeof(identifier_spans) / sizeof(IdentifierSpan);
  static constexpr auto table = identifier_spans;
  using Element = IdentifierSpan;
};

template<typename T>
typename const T::Element* search_table(uint32 code) {
  int right = T::count - 1;
  int left = 0;

  while (left <= right) {
    int mid = (left + right) >> 1;
    const T::Element& span = T::table[mid];

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
  return search_table<WhitespaceData>(code) != nullptr;
}

export bool is_identifier_start(uint32 code) {
  if (code < 128) {
    return
      code >= 'a' && code <= 'z' ||
      code >= 'A' && code <= 'Z' ||
      code == '_' ||
      code == '$';
  }
  auto* span = search_table<IdentifierData>(code);
  return span != nullptr && span->start;
}

export bool is_identifier_part(uint32 code) {
  if (code < 128) {
    return
      code >= 'a' && code <= 'z' ||
      code >= 'A' && code <= 'Z' ||
      code >= '0' && code <= '9' ||
      code == '_' ||
      code == '$';
  }
  return search_table<IdentifierData>(code) != nullptr;
}
