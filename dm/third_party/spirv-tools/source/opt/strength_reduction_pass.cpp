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

#include "source/opt/strength_reduction_pass.h"

#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "source/opt/def_use_manager.h"
#include "source/opt/ir_context.h"
#include "source/opt/log.h"
#include "source/opt/reflect.h"

namespace spvtools {
namespace opt {
namespace {


uint32_t CountTrailingZeros(uint32_t constVal) {


  uint32_t shiftAmount = 0;
  while ((constVal & 1) == 0) {
    ++shiftAmount;
    constVal = (constVal >> 1);
  }
  return shiftAmount;
}


bool IsPowerOf2(uint32_t val) {



  if (val == 0) return false;
  return ((val - 1) & val) == 0;
}

}

Pass::Status StrengthReductionPass::Process() {

  int32_type_id_ = 0;
  uint32_type_id_ = 0;
  std::memset(constant_ids_, 0, sizeof(constant_ids_));

  FindIntTypesAndConstants();
  return ScanFunctions();
}

Pass::Status StrengthReductionPass::ReplaceMultiplyByPowerOf2(
    BasicBlock::iterator* inst) {
  assert((*inst)->opcode() == spv::Op::OpIMul &&
         "Only works for multiplication of integers.");
  bool modified = false;


  if ((*inst)->type_id() != int32_type_id_ &&
      (*inst)->type_id() != uint32_type_id_) {
    return Status::SuccessWithoutChange;
  }


  for (int i = 0; i < 2; i++) {
    uint32_t opId = (*inst)->GetSingleWordInOperand(i);
    Instruction* opInst = get_def_use_mgr()->GetDef(opId);
    if (opInst->opcode() == spv::Op::OpConstant) {

      uint32_t constVal = opInst->GetSingleWordOperand(2);

      if (IsPowerOf2(constVal)) {
        modified = true;
        uint32_t shiftAmount = CountTrailingZeros(constVal);
        uint32_t shiftConstResultId = GetConstantId(shiftAmount);
        if (shiftConstResultId == 0) return Status::Failure;


        uint32_t newResultId = TakeNextId();
        if (newResultId == 0) return Status::Failure;
        std::vector<Operand> newOperands;
        newOperands.push_back((*inst)->GetInOperand(1 - i));
        Operand shiftOperand(spv_operand_type_t::SPV_OPERAND_TYPE_ID,
                             {shiftConstResultId});
        newOperands.push_back(shiftOperand);
        std::unique_ptr<Instruction> newInstruction(
            new Instruction(context(), spv::Op::OpShiftLeftLogical,
                            (*inst)->type_id(), newResultId, newOperands));


        (*inst) = (*inst).InsertBefore(std::move(newInstruction));
        get_def_use_mgr()->AnalyzeInstDefUse(&*(*inst));
        ++(*inst);
        context()->ReplaceAllUsesWith((*inst)->result_id(), newResultId);


        Instruction* inst_to_delete = &*(*inst);
        --(*inst);
        context()->KillInst(inst_to_delete);



        break;
      }
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

void StrengthReductionPass::FindIntTypesAndConstants() {
  analysis::Integer int32(32, true);
  int32_type_id_ = context()->get_type_mgr()->GetId(&int32);
  analysis::Integer uint32(32, false);
  uint32_type_id_ = context()->get_type_mgr()->GetId(&uint32);
  for (auto iter = get_module()->types_values_begin();
       iter != get_module()->types_values_end(); ++iter) {
    switch (iter->opcode()) {
      case spv::Op::OpConstant:
        if (iter->type_id() == uint32_type_id_) {
          uint32_t value = iter->GetSingleWordOperand(2);
          if (value <= 32) constant_ids_[value] = iter->result_id();
        }
        break;
      default:
        break;
    }
  }
}

uint32_t StrengthReductionPass::GetConstantId(uint32_t val) {
  assert(val <= 32 &&
         "This function does not handle constants larger than 32.");

  if (constant_ids_[val] == 0) {
    if (uint32_type_id_ == 0) {
      analysis::Integer uint(32, false);
      uint32_type_id_ = context()->get_type_mgr()->GetTypeInstruction(&uint);
    }


    uint32_t resultId = TakeNextId();
    if (resultId == 0) return 0;
    Operand constant(spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER,
                     {val});
    std::unique_ptr<Instruction> newConstant(new Instruction(
        context(), spv::Op::OpConstant, uint32_type_id_, resultId, {constant}));
    get_module()->AddGlobalValue(std::move(newConstant));


    auto constantIter = --get_module()->types_values_end();
    get_def_use_mgr()->AnalyzeInstDef(&*constantIter);


    constant_ids_[val] = resultId;
  }

  return constant_ids_[val];
}

Pass::Status StrengthReductionPass::ScanFunctions() {



  bool modified = false;
  for (auto& func : *get_module()) {
    for (auto& bb : func) {
      for (auto inst = bb.begin(); inst != bb.end(); ++inst) {
        switch (inst->opcode()) {
          case spv::Op::OpIMul: {
            Status s = ReplaceMultiplyByPowerOf2(&inst);
            if (s == Status::Failure) return Status::Failure;
            if (s == Status::SuccessWithChange) modified = true;
            break;
          }
          default:
            break;
        }
      }
    }
  }
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}
