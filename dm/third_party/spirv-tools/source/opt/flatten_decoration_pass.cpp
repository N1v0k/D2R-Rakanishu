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

#include "source/opt/flatten_decoration_pass.h"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {

using Words = std::vector<uint32_t>;
using OrderedUsesMap = std::unordered_map<uint32_t, Words>;

Pass::Status FlattenDecorationPass::Process() {
  bool modified = false;




  std::unordered_set<uint32_t> group_ids;


  OrderedUsesMap normal_uses;


  OrderedUsesMap member_uses;

  auto annotations = context()->annotations();



  for (const auto& inst : annotations) {
    switch (inst.opcode()) {
      case spv::Op::OpDecorationGroup:
        group_ids.insert(inst.result_id());
        break;
      case spv::Op::OpGroupDecorate: {
        Words& words = normal_uses[inst.GetSingleWordInOperand(0)];
        for (uint32_t i = 1; i < inst.NumInOperandWords(); i++) {
          words.push_back(inst.GetSingleWordInOperand(i));
        }
      } break;
      case spv::Op::OpGroupMemberDecorate: {
        Words& words = member_uses[inst.GetSingleWordInOperand(0)];
        for (uint32_t i = 1; i < inst.NumInOperandWords(); i++) {
          words.push_back(inst.GetSingleWordInOperand(i));
        }
      } break;
      default:
        break;
    }
  }



  auto inst_iter = annotations.begin();

  while (inst_iter != context()->annotations().end()) {

    bool replace = false;
    switch (inst_iter->opcode()) {
      case spv::Op::OpDecorationGroup:
      case spv::Op::OpGroupDecorate:
      case spv::Op::OpGroupMemberDecorate:
        replace = true;
        break;
      case spv::Op::OpDecorate: {


        const uint32_t group = inst_iter->GetSingleWordOperand(0);
        const auto normal_uses_iter = normal_uses.find(group);
        if (normal_uses_iter != normal_uses.end()) {
          for (auto target : normal_uses[group]) {
            std::unique_ptr<Instruction> new_inst(inst_iter->Clone(context()));
            new_inst->SetInOperand(0, Words{target});
            inst_iter = inst_iter.InsertBefore(std::move(new_inst));
            ++inst_iter;
            replace = true;
          }
        }
        const auto member_uses_iter = member_uses.find(group);
        if (member_uses_iter != member_uses.end()) {
          const Words& member_id_pairs = (*member_uses_iter).second;

          assert((member_id_pairs.size() % 2) == 0);
          for (size_t i = 0; i < member_id_pairs.size(); i += 2) {


            const uint32_t target = member_id_pairs[i];
            const uint32_t member = member_id_pairs[i + 1];
            std::vector<Operand> operands;
            operands.push_back(Operand(SPV_OPERAND_TYPE_ID, {target}));
            operands.push_back(
                Operand(SPV_OPERAND_TYPE_LITERAL_INTEGER, {member}));
            auto decoration_operands_iter = inst_iter->begin();
            decoration_operands_iter++;
            operands.insert(operands.end(), decoration_operands_iter,
                            inst_iter->end());
            std::unique_ptr<Instruction> new_inst(new Instruction(
                context(), spv::Op::OpMemberDecorate, 0, 0, operands));
            inst_iter = inst_iter.InsertBefore(std::move(new_inst));
            ++inst_iter;
            replace = true;
          }
        }



        if (!replace && group_ids.count(group)) {
          replace = true;
        }
      } break;
      default:
        break;
    }
    if (replace) {
      inst_iter = inst_iter.Erase();
      modified = true;
    } else {

      ++inst_iter;
    }
  }




  if (!group_ids.empty()) {
    for (auto debug_inst_iter = context()->debug2_begin();
         debug_inst_iter != context()->debug2_end();) {
      if (debug_inst_iter->opcode() == spv::Op::OpName) {
        const uint32_t target = debug_inst_iter->GetSingleWordOperand(0);
        if (group_ids.count(target)) {
          debug_inst_iter = debug_inst_iter.Erase();
          modified = true;
        } else {
          ++debug_inst_iter;
        }
      }
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}
