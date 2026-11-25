// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SOURCE_UTIL_INDEX_RANGE_H_
#define SOURCE_UTIL_INDEX_RANGE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "source/util/span.h"

namespace spvtools {
namespace utils {





template <typename T, class IndexType = uint32_t, class CountType = IndexType>
class IndexRange {
 public:
  static_assert(std::is_integral<IndexType>::value);
  static_assert(std::is_unsigned<IndexType>::value);
  static_assert(std::is_integral<CountType>::value);
  static_assert(std::is_unsigned<CountType>::value);
  using value_type = T;
  using index_type = IndexType;
  using size_type = CountType;

  constexpr IndexRange() {}
  constexpr IndexRange(index_type first, size_type count)
      : first_(first), count_(count) {}

  size_type count() const { return count_; }
  bool empty() const { return count() == size_type(0); }

  IndexType first() const { return first_; }


  template <typename E>
  spvtools::utils::Span<E> apply(E* base) const {
    using span_type = spvtools::utils::Span<E>;
    return base ? span_type(base + first_, count_) : span_type();
  }
  template <typename E = int>
  spvtools::utils::Span<E> apply(std::nullptr_t) const {
    using span_type = spvtools::utils::Span<E>;
    return span_type();
  }

 private:
  index_type first_ = 0;
  size_type count_ = 0;
};

}
}

#endif
