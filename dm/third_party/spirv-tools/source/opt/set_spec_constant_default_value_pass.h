// Copyright (c) 2016 Google Inc.
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

#ifndef SOURCE_OPT_SET_SPEC_CONSTANT_DEFAULT_VALUE_PASS_H_
#define SOURCE_OPT_SET_SPEC_CONSTANT_DEFAULT_VALUE_PASS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class SetSpecConstantDefaultValuePass : public Pass {
 public:
  using SpecIdToValueStrMap = std::unordered_map<uint32_t, std::string>;
  using SpecIdToValueBitPatternMap =
      std::unordered_map<uint32_t, std::vector<uint32_t>>;
  using SpecIdToInstMap = std::unordered_map<uint32_t, Instruction*>;



  explicit SetSpecConstantDefaultValuePass(
      const SpecIdToValueStrMap& default_values)
      : spec_id_to_value_str_(default_values),
        spec_id_to_value_bit_pattern_() {}
  explicit SetSpecConstantDefaultValuePass(SpecIdToValueStrMap&& default_values)
      : spec_id_to_value_str_(std::move(default_values)),
        spec_id_to_value_bit_pattern_() {}



  explicit SetSpecConstantDefaultValuePass(
      const SpecIdToValueBitPatternMap& default_values)
      : spec_id_to_value_str_(),
        spec_id_to_value_bit_pattern_(default_values) {}
  explicit SetSpecConstantDefaultValuePass(
      SpecIdToValueBitPatternMap&& default_values)
      : spec_id_to_value_str_(),
        spec_id_to_value_bit_pattern_(std::move(default_values)) {}

  const char* name() const override { return "set-spec-const-default-value"; }
  Status Process() override;


































  static std::unique_ptr<SpecIdToValueStrMap> ParseDefaultValuesString(
      const char* str);

 private:





  const SpecIdToValueStrMap spec_id_to_value_str_;


  const SpecIdToValueBitPatternMap spec_id_to_value_bit_pattern_;
};

}
}

#endif
