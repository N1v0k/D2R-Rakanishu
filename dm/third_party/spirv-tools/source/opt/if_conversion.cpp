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

#include "source/opt/if_conversion.h"

#include <memory>
#include <vector>

#include "source/opt/value_number_table.h"

namespace spvtools {
namespace opt {

Pass::Status IfConversion::Process() {
  if (!context()->get_feature_mgr()->HasCapability(spv::Capability::Shader)) {
    return Status::SuccessWithoutChange;
  }

  const ValueNumberTable& vn_table = *context()->GetValueNumberTable();
  bool modified = false;
  std::vector<Instruction*> to_kill;
  for (auto& func : *get_module()) {
    DominatorAnalysis* dominators = context()->GetDominatorAnalysis(&func);
    for (auto& block : func) {


      BasicBlock* common = nullptr;
      if (!CheckBlock(&block, dominators, &common)) continue;


      auto iter = block.begin();
      while (iter != block.end() && iter->opcode() == spv::Op::OpPhi) {
        ++iter;
      }

      InstructionBuilder builder(
          context(), &*iter,
          IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
      block.ForEachPhiInst([this, &builder, &modified, &common, &to_kill,
                            dominators, &block, &vn_table](Instruction* phi) {

        if (!CheckType(phi->type_id())) return;





        if (!CheckPhiUsers(phi, &block)) return;





        BasicBlock* inc0 = GetIncomingBlock(phi, 0u);
        Instruction* branch = common->terminator();
        uint32_t condition = branch->GetSingleWordInOperand(0u);
        BasicBlock* then_block = GetBlock(branch->GetSingleWordInOperand(1u));
        Instruction* true_value = nullptr;
        Instruction* false_value = nullptr;
        if ((then_block == &block && inc0 == common) ||
            dominators->Dominates(then_block, inc0)) {
          true_value = GetIncomingValue(phi, 0u);
          false_value = GetIncomingValue(phi, 1u);
        } else {
          true_value = GetIncomingValue(phi, 1u);
          false_value = GetIncomingValue(phi, 0u);
        }

        BasicBlock* true_def_block = context()->get_instr_block(true_value);
        BasicBlock* false_def_block = context()->get_instr_block(false_value);

        uint32_t true_vn = vn_table.GetValueNumber(true_value);
        uint32_t false_vn = vn_table.GetValueNumber(false_value);
        if (true_vn != 0 && true_vn == false_vn) {
          Instruction* inst_to_use = nullptr;




          if (!true_def_block ||
              dominators->Dominates(true_def_block, &block)) {
            inst_to_use = true_value;
          } else if (!false_def_block ||
                     dominators->Dominates(false_def_block, &block)) {
            inst_to_use = false_value;
          } else if (CanHoistInstruction(true_value, common, dominators)) {
            inst_to_use = true_value;
          } else if (CanHoistInstruction(false_value, common, dominators)) {
            inst_to_use = false_value;
          }

          if (inst_to_use != nullptr) {
            modified = true;
            HoistInstruction(inst_to_use, common, dominators);
            context()->KillNamesAndDecorates(phi);
            context()->ReplaceAllUsesWith(phi->result_id(),
                                          inst_to_use->result_id());
          }
          return;
        }





        if (true_def_block && !dominators->Dominates(true_def_block, &block))
          return;

        if (false_def_block && !dominators->Dominates(false_def_block, &block))
          return;

        analysis::Type* data_ty =
            context()->get_type_mgr()->GetType(true_value->type_id());
        if (analysis::Vector* vec_data_ty = data_ty->AsVector()) {
          condition = SplatCondition(vec_data_ty, condition, &builder);
        }


        Instruction* select = builder.AddSelect(phi->type_id(), condition,
                                                true_value->result_id(),
                                                false_value->result_id());
        context()->get_def_use_mgr()->AnalyzeInstDefUse(select);
        select->UpdateDebugInfoFrom(phi);
        context()->ReplaceAllUsesWith(phi->result_id(), select->result_id());
        to_kill.push_back(phi);
        modified = true;

        return;
      });
    }
  }

  for (auto inst : to_kill) {
    context()->KillInst(inst);
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

bool IfConversion::CheckBlock(BasicBlock* block, DominatorAnalysis* dominators,
                              BasicBlock** common) {
  const std::vector<uint32_t>& preds = cfg()->preds(block->id());


  if (preds.size() != 2) return false;

  BasicBlock* inc0 = context()->get_instr_block(preds[0]);
  if (dominators->Dominates(block, inc0)) return false;

  BasicBlock* inc1 = context()->get_instr_block(preds[1]);
  if (dominators->Dominates(block, inc1)) return false;

  if (inc0 == inc1) {


    return false;
  }



  *common = dominators->CommonDominator(inc0, inc1);
  if (!*common || cfg()->IsPseudoEntryBlock(*common)) return false;
  Instruction* branch = (*common)->terminator();
  if (branch->opcode() != spv::Op::OpBranchConditional) return false;
  auto merge = (*common)->GetMergeInst();
  if (!merge || merge->opcode() != spv::Op::OpSelectionMerge) return false;
  if (spv::SelectionControlMask(merge->GetSingleWordInOperand(1)) ==
      spv::SelectionControlMask::DontFlatten) {
    return false;
  }
  if ((*common)->MergeBlockIdIfAny() != block->id()) return false;

  return true;
}

bool IfConversion::CheckPhiUsers(Instruction* phi, BasicBlock* block) {
  return get_def_use_mgr()->WhileEachUser(
      phi, [block, this](Instruction* user) {
        if (user->opcode() == spv::Op::OpPhi &&
            context()->get_instr_block(user) == block)
          return false;
        return true;
      });
}

uint32_t IfConversion::SplatCondition(analysis::Vector* vec_data_ty,
                                      uint32_t cond,
                                      InstructionBuilder* builder) {




  analysis::Bool bool_ty;
  analysis::Vector bool_vec_ty(&bool_ty, vec_data_ty->element_count());
  uint32_t bool_vec_id =
      context()->get_type_mgr()->GetTypeInstruction(&bool_vec_ty);
  std::vector<uint32_t> ids(vec_data_ty->element_count(), cond);

  return builder->AddCompositeConstruct(bool_vec_id, ids)->result_id();
}

bool IfConversion::CheckType(uint32_t id) {
  Instruction* type = get_def_use_mgr()->GetDef(id);
  spv::Op op = type->opcode();
  if (spvOpcodeIsScalarType(op) || op == spv::Op::OpTypePointer ||
      op == spv::Op::OpTypeVector)
    return true;
  return false;
}

BasicBlock* IfConversion::GetBlock(uint32_t id) {
  return context()->get_instr_block(get_def_use_mgr()->GetDef(id));
}

BasicBlock* IfConversion::GetIncomingBlock(Instruction* phi,
                                           uint32_t predecessor) {
  uint32_t in_index = 2 * predecessor + 1;
  return GetBlock(phi->GetSingleWordInOperand(in_index));
}

Instruction* IfConversion::GetIncomingValue(Instruction* phi,
                                            uint32_t predecessor) {
  uint32_t in_index = 2 * predecessor;
  return get_def_use_mgr()->GetDef(phi->GetSingleWordInOperand(in_index));
}

void IfConversion::HoistInstruction(Instruction* inst, BasicBlock* target_block,
                                    DominatorAnalysis* dominators) {
  BasicBlock* inst_block = context()->get_instr_block(inst);
  if (!inst_block) {

    return;
  }

  if (dominators->Dominates(inst_block, target_block)) {

    return;
  }

  assert(inst->IsOpcodeCodeMotionSafe() &&
         "Trying to move an instruction that is not safe to move.");


  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();
  inst->ForEachInId(
      [this, target_block, def_use_mgr, dominators](uint32_t* id) {
        Instruction* operand_inst = def_use_mgr->GetDef(*id);
        HoistInstruction(operand_inst, target_block, dominators);
      });

  Instruction* insertion_pos = target_block->terminator();
  if ((insertion_pos)->PreviousNode()->opcode() == spv::Op::OpSelectionMerge) {
    insertion_pos = insertion_pos->PreviousNode();
  }
  inst->RemoveFromList();
  insertion_pos->InsertBefore(std::unique_ptr<Instruction>(inst));
  context()->set_instr_block(inst, target_block);
}

bool IfConversion::CanHoistInstruction(Instruction* inst,
                                       BasicBlock* target_block,
                                       DominatorAnalysis* dominators) {
  BasicBlock* inst_block = context()->get_instr_block(inst);
  if (!inst_block) {

    return true;
  }

  if (dominators->Dominates(inst_block, target_block)) {

    return true;
  }

  if (!inst->IsOpcodeCodeMotionSafe()) {
    return false;
  }


  analysis::DefUseManager* def_use_mgr = context()->get_def_use_mgr();
  return inst->WhileEachInId(
      [this, target_block, def_use_mgr, dominators](uint32_t* id) {
        Instruction* operand_inst = def_use_mgr->GetDef(*id);
        return CanHoistInstruction(operand_inst, target_block, dominators);
      });
}

}
}
