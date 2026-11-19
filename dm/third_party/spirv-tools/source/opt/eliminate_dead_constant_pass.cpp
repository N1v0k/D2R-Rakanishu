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

#include "source/opt/eliminate_dead_constant_pass.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/def_use_manager.h"
#include "source/opt/log.h"
#include "source/opt/reflect.h"

namespace spvtools {
namespace opt {

Pass::Status EliminateDeadConstantPass::Process() {
  std::unordered_set<Instruction*> working_list;



  std::unordered_map<Instruction*, size_t> use_counts;
  std::vector<Instruction*> constants = context()->GetConstants();
  for (auto* c : constants) {
    uint32_t const_id = c->result_id();
    size_t count = 0;
    context()->get_def_use_mgr()->ForEachUse(
        const_id, [&count](Instruction* user, uint32_t index) {
          (void)index;
          spv::Op op = user->opcode();
          if (!(IsAnnotationInst(op) || IsDebug1Inst(op) || IsDebug2Inst(op) ||
                IsDebug3Inst(op))) {
            ++count;
          }
        });
    use_counts[c] = count;
    if (!count) {
      working_list.insert(c);
    }
  }



  std::unordered_set<Instruction*> dead_consts;
  while (!working_list.empty()) {
    Instruction* inst = *working_list.begin();

    switch (inst->opcode()) {
      case spv::Op::OpConstantComposite:
      case spv::Op::OpSpecConstantComposite:
      case spv::Op::OpSpecConstantOp:
        for (uint32_t i = 0; i < inst->NumInOperands(); i++) {


          if (inst->GetInOperand(i).type != SPV_OPERAND_TYPE_ID) {
            continue;
          }
          uint32_t operand_id = inst->GetSingleWordInOperand(i);
          Instruction* def_inst =
              context()->get_def_use_mgr()->GetDef(operand_id);


          if (!use_counts.count(def_inst)) {
            continue;
          }


          SPIRV_ASSERT(consumer(), use_counts[def_inst] > 0);
          --use_counts[def_inst];
          if (!use_counts[def_inst]) {
            working_list.insert(def_inst);
          }
        }
        break;
      default:
        break;
    }
    dead_consts.insert(inst);
    working_list.erase(inst);
  }


  for (auto* dc : dead_consts) {
    context()->KillDef(dc->result_id());
  }
  return dead_consts.empty() ? Status::SuccessWithoutChange
                             : Status::SuccessWithChange;
}

}
}
