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

#include "source/opt/unify_const_pass.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "source/opt/def_use_manager.h"
#include "source/util/make_unique.h"

namespace spvtools {
namespace opt {
namespace {




class ResultIdTrie {
 public:
  ResultIdTrie() : root_(new Node) {}







  uint32_t LookupEquivalentResultFor(const Instruction& inst) {
    auto keys = GetLookUpKeys(inst);
    auto* node = root_.get();
    for (uint32_t key : keys) {
      node = node->GetOrCreateTrieNodeFor(key);
    }
    if (node->result_id() == 0) {
      node->SetResultId(inst.result_id());
    }
    return node->result_id();
  }

 private:

  class Node {
   public:
    using TrieNodeMap = std::unordered_map<uint32_t, std::unique_ptr<Node>>;

    Node() : result_id_(0), next_() {}
    uint32_t result_id() const { return result_id_; }


    void SetResultId(uint32_t id) { result_id_ = id; }




    Node* GetOrCreateTrieNodeFor(uint32_t key) {
      auto iter = next_.find(key);
      if (iter == next_.end()) {

        return next_.insert(std::make_pair(key, MakeUnique<Node>()))
            .first->second.get();
      }
      return iter->second.get();
    }

   private:

    uint32_t result_id_;

    TrieNodeMap next_;
  };



  std::vector<uint32_t> GetLookUpKeys(const Instruction& inst) {
    std::vector<uint32_t> keys;




    keys.push_back(static_cast<uint32_t>(inst.opcode()));
    for (const auto& operand : inst) {
      if (operand.type == SPV_OPERAND_TYPE_RESULT_ID) continue;
      keys.insert(keys.end(), operand.words.cbegin(), operand.words.cend());
    }
    return keys;
  }

  std::unique_ptr<Node> root_;
};
}

Pass::Status UnifyConstantPass::Process() {
  bool modified = false;
  ResultIdTrie defined_constants;

  for (Instruction *next_instruction,
       *inst = &*(context()->types_values_begin());
       inst; inst = next_instruction) {
    next_instruction = inst->NextNode();



    if (get_def_use_mgr()->GetAnnotations(inst->result_id()).size() != 0) {
      continue;
    }



















    switch (inst->opcode()) {
      case spv::Op::OpConstantTrue:
      case spv::Op::OpConstantFalse:
      case spv::Op::OpConstant:
      case spv::Op::OpConstantNull:
      case spv::Op::OpConstantSampler:
      case spv::Op::OpConstantComposite:









      case spv::Op::OpSpecConstantOp:
      case spv::Op::OpSpecConstantComposite: {
        uint32_t id = defined_constants.LookupEquivalentResultFor(*inst);
        if (id != inst->result_id()) {


          context()->ReplaceAllUsesWith(inst->result_id(), id);
          context()->KillInst(inst);
          modified = true;
        }
        break;
      }
      default:
        break;
    }
  }
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}
