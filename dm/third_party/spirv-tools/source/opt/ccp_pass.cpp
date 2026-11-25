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






#include "source/opt/ccp_pass.h"

#include <algorithm>
#include <limits>

#include "source/opt/fold.h"
#include "source/opt/function.h"
#include "source/opt/propagator.h"

namespace spvtools {
namespace opt {
namespace {



constexpr uint32_t kVaryingSSAId = std::numeric_limits<uint32_t>::max();
}

bool CCPPass::IsVaryingValue(uint32_t id) const { return id == kVaryingSSAId; }

SSAPropagator::PropStatus CCPPass::MarkInstructionVarying(Instruction* instr) {
  assert(instr->result_id() != 0 &&
         "Instructions with no result cannot be marked varying.");
  values_[instr->result_id()] = kVaryingSSAId;
  return SSAPropagator::kVarying;
}

SSAPropagator::PropStatus CCPPass::VisitPhi(Instruction* phi) {
  uint32_t meet_val_id = 0;




  for (uint32_t i = 2; i < phi->NumOperands(); i += 2) {
    if (!propagator_->IsPhiArgExecutable(phi, i)) {

      continue;
    }
    uint32_t phi_arg_id = phi->GetSingleWordOperand(i);
    auto it = values_.find(phi_arg_id);
    if (it != values_.end()) {


      if (it->second == kVaryingSSAId) {


        return MarkInstructionVarying(phi);
      } else if (meet_val_id == 0) {


        meet_val_id = it->second;
      } else if (it->second == meet_val_id) {


        continue;
      } else {



        return MarkInstructionVarying(phi);
      }
    } else {



      continue;
    }
  }



  if (meet_val_id == 0) {
    return SSAPropagator::kNotInteresting;
  }



  values_[phi->result_id()] = meet_val_id;
  return SSAPropagator::kInteresting;
}

uint32_t CCPPass::ComputeLatticeMeet(Instruction* instr, uint32_t val2) {











  auto val1_it = values_.find(instr->result_id());
  if (val1_it == values_.end()) {
    return val2;
  }

  uint32_t val1 = val1_it->second;
  if (IsVaryingValue(val1)) {
    return val1;
  } else if (IsVaryingValue(val2)) {
    return val2;
  } else if (val1 != val2) {
    return kVaryingSSAId;
  }
  return val2;
}

SSAPropagator::PropStatus CCPPass::VisitAssignment(Instruction* instr) {
  assert(instr->result_id() != 0 &&
         "Expecting an instruction that produces a result");



  if (instr->opcode() == spv::Op::OpCopyObject) {
    uint32_t rhs_id = instr->GetSingleWordInOperand(0);
    auto it = values_.find(rhs_id);
    if (it != values_.end()) {
      if (IsVaryingValue(it->second)) {
        return MarkInstructionVarying(instr);
      } else {
        uint32_t new_val = ComputeLatticeMeet(instr, it->second);
        values_[instr->result_id()] = new_val;
        return IsVaryingValue(new_val) ? SSAPropagator::kVarying
                                       : SSAPropagator::kInteresting;
      }
    }
    return SSAPropagator::kNotInteresting;
  }


  if (!instr->IsFoldable()) {
    return MarkInstructionVarying(instr);
  }


  auto map_func = [this](uint32_t id) {
    auto it = values_.find(id);
    if (it == values_.end() || IsVaryingValue(it->second)) {
      return id;
    }
    return it->second;
  };
  Instruction* folded_inst =
      context()->get_instruction_folder().FoldInstructionToConstant(instr,
                                                                    map_func);

  if (folded_inst && context()->id_overflow()) {
    return SSAPropagator::kFailed;
  }
  if (folded_inst != nullptr) {


    assert((folded_inst->IsConstant() ||
            IsSpecConstantInst(folded_inst->opcode())) &&
           "CCP is only interested in constant values.");
    uint32_t new_val = ComputeLatticeMeet(instr, folded_inst->result_id());
    values_[instr->result_id()] = new_val;
    return IsVaryingValue(new_val) ? SSAPropagator::kVarying
                                   : SSAPropagator::kInteresting;
  }


  if (!instr->WhileEachInId([this](uint32_t* op_id) {
        auto iter = values_.find(*op_id);
        if (iter != values_.end() && IsVaryingValue(iter->second)) return false;
        return true;
      })) {
    return MarkInstructionVarying(instr);
  }



  if (!instr->WhileEachInId([this](uint32_t* op_id) {
        auto it = values_.find(*op_id);
        if (it == values_.end()) return false;
        return true;
      })) {
    return SSAPropagator::kNotInteresting;
  }



  return MarkInstructionVarying(instr);
}

SSAPropagator::PropStatus CCPPass::VisitBranch(Instruction* instr,
                                               BasicBlock** dest_bb) const {
  assert(instr->IsBranch() && "Expected a branch instruction.");

  *dest_bb = nullptr;
  uint32_t dest_label = 0;
  if (instr->opcode() == spv::Op::OpBranch) {

    dest_label = instr->GetSingleWordInOperand(0);
  } else if (instr->opcode() == spv::Op::OpBranchConditional) {



    uint32_t pred_id = instr->GetSingleWordOperand(0);
    auto it = values_.find(pred_id);
    if (it == values_.end() || IsVaryingValue(it->second)) {

      return SSAPropagator::kVarying;
    }



    uint32_t pred_val_id = it->second;
    const analysis::Constant* c = const_mgr_->FindDeclaredConstant(pred_val_id);
    assert(c && "Expected to find a constant declaration for a known value.");

    assert(c->AsBoolConstant() || c->AsNullConstant());
    if (c->AsNullConstant()) {
      dest_label = instr->GetSingleWordOperand(2u);
    } else {
      const analysis::BoolConstant* val = c->AsBoolConstant();
      dest_label = val->value() ? instr->GetSingleWordOperand(1)
                                : instr->GetSingleWordOperand(2);
    }
  } else {



    assert(instr->opcode() == spv::Op::OpSwitch);
    if (instr->GetOperand(0).words.size() != 1) {


      return SSAPropagator::kVarying;
    }
    uint32_t select_id = instr->GetSingleWordOperand(0);
    auto it = values_.find(select_id);
    if (it == values_.end() || IsVaryingValue(it->second)) {

      return SSAPropagator::kVarying;
    }



    uint32_t select_val_id = it->second;
    const analysis::Constant* c =
        const_mgr_->FindDeclaredConstant(select_val_id);
    assert(c && "Expected to find a constant declaration for a known value.");

    uint32_t constant_cond = 0;
    if (const analysis::IntConstant* val = c->AsIntConstant()) {
      constant_cond = val->words()[0];
    } else {

      assert(c->AsNullConstant());
      constant_cond = 0;
    }


    dest_label = instr->GetSingleWordOperand(1);
    for (uint32_t i = 2; i < instr->NumOperands(); i += 2) {
      if (constant_cond == instr->GetSingleWordOperand(i)) {
        dest_label = instr->GetSingleWordOperand(i + 1);
        break;
      }
    }
  }

  assert(dest_label && "Destination label should be set at this point.");
  *dest_bb = context()->cfg()->block(dest_label);
  return SSAPropagator::kInteresting;
}

SSAPropagator::PropStatus CCPPass::VisitInstruction(Instruction* instr,
                                                    BasicBlock** dest_bb) {
  *dest_bb = nullptr;
  if (instr->opcode() == spv::Op::OpPhi) {
    return VisitPhi(instr);
  } else if (instr->IsBranch()) {
    return VisitBranch(instr, dest_bb);
  } else if (instr->result_id()) {
    return VisitAssignment(instr);
  }
  return SSAPropagator::kVarying;
}

bool CCPPass::ReplaceValues() {









  bool changed_ir = (context()->module()->IdBound() > original_id_bound_);

  for (const auto& it : values_) {
    uint32_t id = it.first;
    uint32_t cst_id = it.second;
    if (!IsVaryingValue(cst_id) && id != cst_id) {
      context()->KillNamesAndDecorates(id);
      changed_ir |= context()->ReplaceAllUsesWith(id, cst_id);
    }
  }

  return changed_ir;
}

bool CCPPass::PropagateConstants(Function* fp) {
  if (fp->IsDeclaration()) {
    return false;
  }


  fp->ForEachParam([this](const Instruction* inst) {
    values_[inst->result_id()] = kVaryingSSAId;
  });

  const auto visit_fn = [this](Instruction* instr, BasicBlock** dest_bb) {
    return VisitInstruction(instr, dest_bb);
  };

  propagator_ =
      std::unique_ptr<SSAPropagator>(new SSAPropagator(context(), visit_fn));

  if (propagator_->Run(fp)) {
    return ReplaceValues();
  }

  return false;
}

void CCPPass::Initialize() {
  const_mgr_ = context()->get_constant_mgr();




  for (const auto& inst : get_module()->types_values()) {


    if (inst.IsConstant()) {
      values_[inst.result_id()] = inst.result_id();
    } else {
      values_[inst.result_id()] = kVaryingSSAId;
    }
  }




  for (const auto& inst : get_module()->ext_inst_imports()) {
    values_[inst.result_id()] = kVaryingSSAId;
  }

  original_id_bound_ = context()->module()->IdBound();
}

Pass::Status CCPPass::Process() {
  Initialize();


  ProcessFunction pfn = [this](Function* fp) { return PropagateConstants(fp); };
  bool modified = context()->ProcessReachableCallTree(pfn);
  if (context()->id_overflow()) return Pass::Status::Failure;
  return modified ? Pass::Status::SuccessWithChange
                  : Pass::Status::SuccessWithoutChange;
}

}
}
