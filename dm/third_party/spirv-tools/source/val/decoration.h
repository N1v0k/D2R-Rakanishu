// Copyright (c) 2017 Google Inc.
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

#ifndef SOURCE_VAL_DECORATION_H_
#define SOURCE_VAL_DECORATION_H_

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "source/latest_version_spirv_header.h"

namespace spvtools {
namespace val {






































class Decoration {
 public:
  enum { kInvalidMember = -1 };
  Decoration(spv::Decoration t,
             const std::vector<uint32_t>& parameters = std::vector<uint32_t>(),
             uint32_t member_index = kInvalidMember)
      : dec_type_(t), params_(parameters), struct_member_index_(member_index) {}

  void set_struct_member_index(uint32_t index) { struct_member_index_ = index; }
  int struct_member_index() const { return struct_member_index_; }
  spv::Decoration dec_type() const { return dec_type_; }
  std::vector<uint32_t>& params() { return params_; }
  const std::vector<uint32_t>& params() const { return params_; }
  spv::BuiltIn builtin() const {
    assert(dec_type_ == spv::Decoration::BuiltIn);
    return spv::BuiltIn(params_[0]);
  }

  inline bool operator<(const Decoration& rhs) const {


    if (struct_member_index_ < rhs.struct_member_index_) return true;
    if (rhs.struct_member_index_ < struct_member_index_) return false;
    if (dec_type_ < rhs.dec_type_) return true;
    if (rhs.dec_type_ < dec_type_) return false;
    return params_ < rhs.params_;
  }
  inline bool operator==(const Decoration& rhs) const {
    return (dec_type_ == rhs.dec_type_ && params_ == rhs.params_ &&
            struct_member_index_ == rhs.struct_member_index_);
  }

 private:
  spv::Decoration dec_type_;
  std::vector<uint32_t> params_;



  int struct_member_index_;
};

}
}

#endif
