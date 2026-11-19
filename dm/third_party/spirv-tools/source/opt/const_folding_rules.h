// Copyright (c) 2018 Google LLC
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

#ifndef SOURCE_OPT_CONST_FOLDING_RULES_H_
#define SOURCE_OPT_CONST_FOLDING_RULES_H_

#include <unordered_map>
#include <vector>

#include "source/opt/constants.h"

namespace spvtools {
namespace opt {


























using ConstantFoldingRule = std::function<const analysis::Constant*(
    IRContext* ctx, Instruction* inst,
    const std::vector<const analysis::Constant*>& constants)>;

class ConstantFoldingRules {
 protected:


  struct Key {
    uint32_t instruction_set;
    uint32_t opcode;
  };

  friend bool operator<(const Key& a, const Key& b) {
    if (a.instruction_set < b.instruction_set) {
      return true;
    }
    if (a.instruction_set > b.instruction_set) {
      return false;
    }
    return a.opcode < b.opcode;
  }

  struct Value {
    std::vector<ConstantFoldingRule> value;
    void push_back(ConstantFoldingRule rule) { value.push_back(rule); }
  };

 public:
  ConstantFoldingRules(IRContext* ctx) : context_(ctx) {}
  virtual ~ConstantFoldingRules() = default;


  bool HasFoldingRule(const Instruction* inst) const {
    return !GetRulesForInstruction(inst).empty();
  }


  const std::vector<ConstantFoldingRule>& GetRulesForInstruction(
      const Instruction* inst) const {
    if (inst->opcode() != spv::Op::OpExtInst) {
      auto it = rules_.find(inst->opcode());
      if (it != rules_.end()) {
        return it->second.value;
      }
    } else {
      uint32_t ext_inst_id = inst->GetSingleWordInOperand(0);
      uint32_t ext_opcode = inst->GetSingleWordInOperand(1);
      auto it = ext_rules_.find({ext_inst_id, ext_opcode});
      if (it != ext_rules_.end()) {
        return it->second.value;
      }
    }
    return empty_vector_;
  }


  virtual void AddFoldingRules();

 protected:
  struct hasher {
    size_t operator()(const spv::Op& op) const noexcept {
      return std::hash<uint32_t>()(uint32_t(op));
    }
  };



  std::unordered_map<spv::Op, Value, hasher> rules_;


  std::map<Key, Value> ext_rules_;

 private:

  IRContext* context_;



  std::vector<ConstantFoldingRule> empty_vector_;
};

}
}

#endif
