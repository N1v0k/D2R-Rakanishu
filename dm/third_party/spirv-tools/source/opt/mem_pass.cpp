// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#include "source/opt/mem_pass.h"

#include <memory>
#include <set>
#include <vector>

#include "source/cfa.h"
#include "source/opt/basic_block.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {
namespace {
constexpr uint32_t kCopyObjectOperandInIdx = 0;
constexpr uint32_t kTypePointerStorageClassInIdx = 0;
constexpr uint32_t kTypePointerTypeIdInIdx = 1;
}

bool MemPass::IsBaseTargetType(const Instruction* typeInst) const {
  switch (typeInst->opcode()) {
    case spv::Op::OpTypeInt:
    case spv::Op::OpTypeFloat:
    case spv::Op::OpTypeBool:
    case spv::Op::OpTypeVector:
    case spv::Op::OpTypeMatrix:
    case spv::Op::OpTypeImage:
    case spv::Op::OpTypeSampler:
    case spv::Op::OpTypeSampledImage:
    case spv::Op::OpTypePointer:
    case spv::Op::OpTypeCooperativeMatrixNV:
    case spv::Op::OpTypeCooperativeMatrixKHR:
      return true;
    default:
      break;
  }
  return false;
}

bool MemPass::IsTargetType(const Instruction* typeInst) const {
  if (IsBaseTargetType(typeInst)) return true;
  if (typeInst->opcode() == spv::Op::OpTypeArray) {
    if (!IsTargetType(
            get_def_use_mgr()->GetDef(typeInst->GetSingleWordOperand(1)))) {
      return false;
    }
    return true;
  }
  if (typeInst->opcode() != spv::Op::OpTypeStruct) return false;

  return typeInst->WhileEachInId([this](const uint32_t* tid) {
    Instruction* compTypeInst = get_def_use_mgr()->GetDef(*tid);
    if (!IsTargetType(compTypeInst)) return false;
    return true;
  });
}

bool MemPass::IsNonPtrAccessChain(const spv::Op opcode) const {
  return opcode == spv::Op::OpAccessChain ||
         opcode == spv::Op::OpInBoundsAccessChain ||
         opcode == spv::Op::OpUntypedAccessChainKHR;
}

bool MemPass::IsPtr(uint32_t ptrId) {
  uint32_t varId = ptrId;
  Instruction* ptrInst = get_def_use_mgr()->GetDef(varId);
  if (ptrInst->opcode() == spv::Op::OpFunction) {


    return false;
  }
  while (ptrInst->opcode() == spv::Op::OpCopyObject) {
    varId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = get_def_use_mgr()->GetDef(varId);
  }
  const spv::Op op = ptrInst->opcode();
  if (op == spv::Op::OpVariable || op == spv::Op::OpUntypedVariableKHR ||
      IsNonPtrAccessChain(op))
    return true;
  const uint32_t varTypeId = ptrInst->type_id();
  if (varTypeId == 0) return false;
  const Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  return varTypeInst->opcode() == spv::Op::OpTypePointer ||
         varTypeInst->opcode() == spv::Op::OpTypeUntypedPointerKHR;
}

Instruction* MemPass::GetPtr(uint32_t ptrId, uint32_t* varId) {
  *varId = ptrId;
  Instruction* ptrInst = get_def_use_mgr()->GetDef(*varId);
  Instruction* varInst;

  switch (ptrInst->opcode()) {
    case spv::Op::OpVariable:
    case spv::Op::OpUntypedVariableKHR:
    case spv::Op::OpFunctionParameter:
      varInst = ptrInst;
      break;
    case spv::Op::OpAccessChain:
    case spv::Op::OpInBoundsAccessChain:
    case spv::Op::OpUntypedAccessChainKHR:
    case spv::Op::OpPtrAccessChain:
    case spv::Op::OpInBoundsPtrAccessChain:
    case spv::Op::OpImageTexelPointer:
    case spv::Op::OpCopyObject:
      varInst = ptrInst->GetBaseAddress();
      break;
    default:
      *varId = 0;
      return ptrInst;
      break;
  }

  if (varInst->opcode() == spv::Op::OpVariable ||
      varInst->opcode() == spv::Op::OpUntypedVariableKHR) {
    *varId = varInst->result_id();
  } else {
    *varId = 0;
  }

  while (ptrInst->opcode() == spv::Op::OpCopyObject) {
    uint32_t temp = ptrInst->GetSingleWordInOperand(0);
    ptrInst = get_def_use_mgr()->GetDef(temp);
  }

  return ptrInst;
}

Instruction* MemPass::GetPtr(Instruction* ip, uint32_t* varId) {
  assert(ip->opcode() == spv::Op::OpStore || ip->opcode() == spv::Op::OpLoad ||
         ip->opcode() == spv::Op::OpImageTexelPointer ||
         ip->IsAtomicWithLoad());


  const uint32_t ptrId = ip->GetSingleWordInOperand(0);
  return GetPtr(ptrId, varId);
}

bool MemPass::HasOnlyNamesAndDecorates(uint32_t id) const {
  return get_def_use_mgr()->WhileEachUser(id, [this](Instruction* user) {
    spv::Op op = user->opcode();
    if (op != spv::Op::OpName && !IsNonTypeDecorate(op)) {
      return false;
    }
    return true;
  });
}

void MemPass::KillAllInsts(BasicBlock* bp, bool killLabel) {
  bp->KillAllInsts(killLabel);
}

bool MemPass::HasLoads(uint32_t varId) const {
  return !get_def_use_mgr()->WhileEachUser(varId, [this](Instruction* user) {
    spv::Op op = user->opcode();


    if (IsNonPtrAccessChain(op) || op == spv::Op::OpCopyObject) {
      if (HasLoads(user->result_id())) {
        return false;
      }
    } else if (op != spv::Op::OpStore && op != spv::Op::OpName &&
               !IsNonTypeDecorate(op)) {
      return false;
    }
    return true;
  });
}

bool MemPass::IsLiveVar(uint32_t varId) const {
  const Instruction* varInst = get_def_use_mgr()->GetDef(varId);

  if (varInst->opcode() != spv::Op::OpVariable) return true;

  const uint32_t varTypeId = varInst->type_id();
  const Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  if (spv::StorageClass(varTypeInst->GetSingleWordInOperand(
          kTypePointerStorageClassInIdx)) != spv::StorageClass::Function)
    return true;

  return HasLoads(varId);
}

void MemPass::AddStores(uint32_t ptr_id, std::queue<Instruction*>* insts) {
  get_def_use_mgr()->ForEachUser(ptr_id, [this, insts](Instruction* user) {
    spv::Op op = user->opcode();
    if (IsNonPtrAccessChain(op)) {
      AddStores(user->result_id(), insts);
    } else if (op == spv::Op::OpStore) {
      insts->push(user);
    }
  });
}

void MemPass::DCEInst(Instruction* inst,
                      const std::function<void(Instruction*)>& call_back) {
  std::queue<Instruction*> deadInsts;
  deadInsts.push(inst);
  while (!deadInsts.empty()) {
    Instruction* di = deadInsts.front();

    if (di->opcode() == spv::Op::OpLabel) {
      deadInsts.pop();
      continue;
    }

    std::set<uint32_t> ids;
    di->ForEachInId([&ids](uint32_t* iid) { ids.insert(*iid); });
    uint32_t varId = 0;

    if (di->opcode() == spv::Op::OpLoad) (void)GetPtr(di, &varId);
    if (call_back) {
      call_back(di);
    }
    context()->KillInst(di);


    for (auto id : ids)
      if (HasOnlyNamesAndDecorates(id)) {
        Instruction* odi = get_def_use_mgr()->GetDef(id);
        if (context()->IsCombinatorInstruction(odi)) deadInsts.push(odi);
      }


    if (varId != 0 && !IsLiveVar(varId)) AddStores(varId, &deadInsts);
    deadInsts.pop();
  }
}

MemPass::MemPass() {}

bool MemPass::HasOnlySupportedRefs(uint32_t varId) {
  return get_def_use_mgr()->WhileEachUser(varId, [this](Instruction* user) {
    auto dbg_op = user->GetCommonDebugOpcode();
    if (dbg_op == CommonDebugInfoDebugDeclare ||
        dbg_op == CommonDebugInfoDebugValue) {
      return true;
    }
    spv::Op op = user->opcode();
    if (op != spv::Op::OpStore && op != spv::Op::OpLoad &&
        op != spv::Op::OpName && !IsNonTypeDecorate(op)) {
      return false;
    }
    return true;
  });
}

uint32_t MemPass::Type2Undef(uint32_t type_id) {
  const auto uitr = type2undefs_.find(type_id);
  if (uitr != type2undefs_.end()) return uitr->second;
  const uint32_t undefId = TakeNextId();
  if (undefId == 0) {
    return 0;
  }

  std::unique_ptr<Instruction> undef_inst(
      new Instruction(context(), spv::Op::OpUndef, type_id, undefId, {}));
  get_def_use_mgr()->AnalyzeInstDefUse(&*undef_inst);
  get_module()->AddGlobalValue(std::move(undef_inst));
  type2undefs_[type_id] = undefId;
  return undefId;
}

bool MemPass::IsTargetVar(uint32_t varId) {
  if (varId == 0) {
    return false;
  }

  if (seen_non_target_vars_.find(varId) != seen_non_target_vars_.end())
    return false;
  if (seen_target_vars_.find(varId) != seen_target_vars_.end()) return true;
  const Instruction* varInst = get_def_use_mgr()->GetDef(varId);
  if (varInst->opcode() != spv::Op::OpVariable) return false;
  const uint32_t varTypeId = varInst->type_id();
  const Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  if (spv::StorageClass(varTypeInst->GetSingleWordInOperand(
          kTypePointerStorageClassInIdx)) != spv::StorageClass::Function) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  const uint32_t varPteTypeId =
      varTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
  Instruction* varPteTypeInst = get_def_use_mgr()->GetDef(varPteTypeId);
  if (!IsTargetType(varPteTypeInst)) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  seen_target_vars_.insert(varId);
  return true;
}













































bool MemPass::RemovePhiOperands(
    Instruction* phi, const std::unordered_set<BasicBlock*>& reachable_blocks) {
  std::vector<Operand> keep_operands;
  uint32_t type_id = 0;

  uint32_t undef_id = 0;



  for (uint32_t i = 0; i < phi->NumOperands();) {
    if (i < 2) {

      keep_operands.push_back(phi->GetOperand(i));
      ++i;
      continue;
    }



    assert(i % 2 == 0 && i < phi->NumOperands() - 1 &&
           "malformed Phi arguments");

    BasicBlock* in_block = cfg()->block(phi->GetSingleWordOperand(i + 1));
    if (reachable_blocks.find(in_block) == reachable_blocks.end()) {


      i += 2;
      continue;
    }


    uint32_t arg_id = phi->GetSingleWordOperand(i);
    Instruction* arg_def_instr = get_def_use_mgr()->GetDef(arg_id);
    BasicBlock* def_block = context()->get_instr_block(arg_def_instr);
    if (def_block &&
        reachable_blocks.find(def_block) == reachable_blocks.end()) {



      if (!undef_id) {
        type_id = arg_def_instr->type_id();
        undef_id = Type2Undef(type_id);
        if (undef_id == 0) return false;
      }
      keep_operands.push_back(
          Operand(spv_operand_type_t::SPV_OPERAND_TYPE_ID, {undef_id}));
    } else {



      keep_operands.push_back(phi->GetOperand(i));
    }

    keep_operands.push_back(phi->GetOperand(i + 1));

    i += 2;
  }

  context()->ForgetUses(phi);
  phi->ReplaceOperands(keep_operands);
  context()->AnalyzeUses(phi);
  return true;
}

void MemPass::RemoveBlock(Function::iterator* bi) {
  auto& rm_block = **bi;


  rm_block.ForEachInst([&rm_block, this](Instruction* inst) {



    if (inst != rm_block.GetLabelInst()) {
      context()->KillInst(inst);
    }
  });


  auto label = rm_block.GetLabelInst();
  context()->KillInst(label);

  *bi = bi->Erase();
}

Pass::Status MemPass::RemoveUnreachableBlocks(Function* func) {
  if (func->IsDeclaration()) return Status::SuccessWithoutChange;
  bool modified = false;


  std::unordered_set<BasicBlock*> reachable_blocks;
  std::unordered_set<BasicBlock*> visited_blocks;
  std::queue<BasicBlock*> worklist;
  reachable_blocks.insert(func->entry().get());


  worklist.push(func->entry().get());

  auto mark_reachable = [&reachable_blocks, &visited_blocks, &worklist,
                         this](uint32_t label_id) {
    auto successor = cfg()->block(label_id);
    if (visited_blocks.count(successor) == 0) {
      reachable_blocks.insert(successor);
      worklist.push(successor);
      visited_blocks.insert(successor);
    }
  };


  while (!worklist.empty()) {
    BasicBlock* block = worklist.front();
    worklist.pop();


    static_cast<const BasicBlock*>(block)->ForEachSuccessorLabel(
        mark_reachable);


    block->ForMergeAndContinueLabel(mark_reachable);
  }


  for (auto& block : *func) {


    if (reachable_blocks.count(&block) == 0) {
      continue;
    }




    bool success =
        block.WhileEachPhiInst([&reachable_blocks, this](Instruction* phi) {
          return RemovePhiOperands(phi, reachable_blocks);
        });
    if (!success) return Status::Failure;
  }


  for (auto ebi = func->begin(); ebi != func->end();) {
    if (reachable_blocks.count(&*ebi) == 0) {
      RemoveBlock(&ebi);
      modified = true;
    } else {
      ++ebi;
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

Pass::Status MemPass::CFGCleanup(Function* func) {
  return RemoveUnreachableBlocks(func);
}

void MemPass::CollectTargetVars(Function* func) {
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();
  type2undefs_.clear();



  for (auto& blk : *func) {
    for (auto& inst : blk) {
      switch (inst.opcode()) {
        case spv::Op::OpStore:
        case spv::Op::OpLoad: {
          uint32_t varId;
          (void)GetPtr(&inst, &varId);
          if (!IsTargetVar(varId)) break;
          if (HasOnlySupportedRefs(varId)) break;
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
        } break;
        default:
          break;
      }
    }
  }
}

}
}
