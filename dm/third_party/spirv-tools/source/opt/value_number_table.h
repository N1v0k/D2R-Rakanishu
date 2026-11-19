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

#ifndef SOURCE_OPT_VALUE_NUMBER_TABLE_H_
#define SOURCE_OPT_VALUE_NUMBER_TABLE_H_

#include <cstdint>
#include <unordered_map>

#include "source/opt/instruction.h"

namespace spvtools {
namespace opt {

class IRContext;



class ComputeSameValue {
 public:
  bool operator()(const Instruction& lhs, const Instruction& rhs) const;
};


class ValueTableHash {
 public:
  std::size_t operator()(const Instruction& inst) const;
};












class ValueNumberTable {
 public:
  ValueNumberTable(IRContext* ctx) : context_(ctx), next_value_number_(1) {
    BuildDominatorTreeValueNumberTable();
  }




  uint32_t GetValueNumber(Instruction* inst) const;



  uint32_t GetValueNumber(uint32_t id) const;

  IRContext* context() const { return context_; }

 private:

  void BuildDominatorTreeValueNumberTable();



  bool IsReadOnlyLoad(Instruction* inst);



  bool IsReadOnlyVariable(Instruction* address_def);


  uint32_t TakeNextValueNumber() { return next_value_number_++; }




  uint32_t AssignValueNumber(Instruction* inst);

  std::unordered_map<Instruction, uint32_t, ValueTableHash, ComputeSameValue>
      instruction_to_value_;
  std::unordered_map<uint32_t, uint32_t> id_to_value_;


  std::unordered_map<uint32_t, bool> read_only_variable_cache_;

  IRContext* context_;
  uint32_t next_value_number_;
};

}
}

#endif
