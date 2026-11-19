// Copyright (c) 2019 Google LLC
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

#include "code_sink.h"

#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/ir_context.h"
#include "source/util/bit_vector.h"

namespace spvtools {
namespace opt {

Pass::Status CodeSinkingPass::Process() {
  bool modified = false;
  for (Function& function : *get_module()) {
    cfg()->ForEachBlockInPostOrder(function.entry().get(),
                                   [&modified, this](BasicBlock* bb) {
                                     if (SinkInstructionsInBB(bb)) {
                                       modified = true;
                                     }
                                   });
  }
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

bool CodeSinkingPass::SinkInstructionsInBB(BasicBlock* bb) {
  bool modified = false;
  for (auto inst = bb->rbegin(); inst != bb->rend(); ++inst) {
    if (SinkInstruction(&*inst)) {
      inst = bb->rbegin();
      modified = true;
    }
  }
  return modified;
}

bool CodeSinkingPass::SinkInstruction(Instruction* inst) {
  if (inst->opcode() != spv::Op::OpLoad &&
      inst->opcode() != spv::Op::OpAccessChain) {
    return false;
  }

  if (ReferencesMutableMemory(inst)) {
    return false;
  }

  if (BasicBlock* target_bb = FindNewBasicBlockFor(inst)) {
    Instruction* pos = &*target_bb->begin();
    while (pos->opcode() == spv::Op::OpPhi) {
      pos = pos->NextNode();
    }

    inst->InsertBefore(pos);
    context()->set_instr_block(inst, target_bb);
    return true;
  }
  return false;
}

BasicBlock* CodeSinkingPass::FindNewBasicBlockFor(Instruction* inst) {
  assert(inst->result_id() != 0 && "Instruction should have a result.");
  BasicBlock* original_bb = context()->get_instr_block(inst);
  BasicBlock* bb = original_bb;

  std::unordered_set<uint32_t> bbs_with_uses;
  get_def_use_mgr()->ForEachUse(
      inst, [&bbs_with_uses, this](Instruction* use, uint32_t idx) {
        if (use->opcode() != spv::Op::OpPhi) {
          BasicBlock* use_bb = context()->get_instr_block(use);
          if (use_bb) {
            bbs_with_uses.insert(use_bb->id());
          }
        } else {
          bbs_with_uses.insert(use->GetSingleWordOperand(idx + 1));
        }
      });

  while (true) {

    if (bbs_with_uses.count(bb->id())) {
      break;
    }





    if (bb->terminator()->opcode() == spv::Op::OpBranch) {
      uint32_t succ_bb_id = bb->terminator()->GetSingleWordInOperand(0);
      if (cfg()->preds(succ_bb_id).size() == 1) {
        bb = context()->get_instr_block(succ_bb_id);
        continue;
      } else {
        break;
      }
    }




    Instruction* merge_inst = bb->GetMergeInst();
    if (merge_inst == nullptr ||
        merge_inst->opcode() != spv::Op::OpSelectionMerge) {
      break;
    }



    bool used_in_multiple_blocks = false;
    uint32_t bb_used_in = 0;
    bb->ForEachSuccessorLabel([this, bb, &bb_used_in, &used_in_multiple_blocks,
                               &bbs_with_uses](uint32_t* succ_bb_id) {
      if (IntersectsPath(*succ_bb_id, bb->MergeBlockIdIfAny(), bbs_with_uses)) {
        if (bb_used_in == 0) {
          bb_used_in = *succ_bb_id;
        } else {
          used_in_multiple_blocks = true;
        }
      }
    });




    if (used_in_multiple_blocks) {
      break;
    }

    if (bb_used_in == 0) {


      bb = context()->get_instr_block(bb->MergeBlockIdIfAny());
    } else {



      if (cfg()->preds(bb_used_in).size() != 1) {
        break;
      }



      if (IntersectsPath(bb->MergeBlockIdIfAny(), original_bb->id(),
                         bbs_with_uses)) {
        break;
      }



      bb = context()->get_instr_block(bb_used_in);
    }
    continue;
  }
  return (bb != original_bb ? bb : nullptr);
}

bool CodeSinkingPass::ReferencesMutableMemory(Instruction* inst) {
  if (!inst->IsLoad()) {
    return false;
  }

  Instruction* base_ptr = inst->GetBaseAddress();
  if (base_ptr->opcode() != spv::Op::OpVariable) {
    return true;
  }

  if (base_ptr->IsReadOnlyPointer()) {
    return false;
  }

  if (HasUniformMemorySync()) {
    return true;
  }

  if (spv::StorageClass(base_ptr->GetSingleWordInOperand(0)) !=
      spv::StorageClass::Uniform) {
    return true;
  }

  return HasPossibleStore(base_ptr);
}

bool CodeSinkingPass::HasUniformMemorySync() {
  if (checked_for_uniform_sync_) {
    return has_uniform_sync_;
  }

  bool has_sync = false;
  get_module()->ForEachInst([this, &has_sync](Instruction* inst) {
    switch (inst->opcode()) {
      case spv::Op::OpMemoryBarrier: {
        uint32_t mem_semantics_id = inst->GetSingleWordInOperand(1);
        if (IsSyncOnUniform(mem_semantics_id)) {
          has_sync = true;
        }
        break;
      }
      case spv::Op::OpControlBarrier:
      case spv::Op::OpAtomicLoad:
      case spv::Op::OpAtomicStore:
      case spv::Op::OpAtomicExchange:
      case spv::Op::OpAtomicIIncrement:
      case spv::Op::OpAtomicIDecrement:
      case spv::Op::OpAtomicIAdd:
      case spv::Op::OpAtomicFAddEXT:
      case spv::Op::OpAtomicISub:
      case spv::Op::OpAtomicSMin:
      case spv::Op::OpAtomicUMin:
      case spv::Op::OpAtomicFMinEXT:
      case spv::Op::OpAtomicSMax:
      case spv::Op::OpAtomicUMax:
      case spv::Op::OpAtomicFMaxEXT:
      case spv::Op::OpAtomicAnd:
      case spv::Op::OpAtomicOr:
      case spv::Op::OpAtomicXor:
      case spv::Op::OpAtomicFlagTestAndSet:
      case spv::Op::OpAtomicFlagClear: {
        uint32_t mem_semantics_id = inst->GetSingleWordInOperand(2);
        if (IsSyncOnUniform(mem_semantics_id)) {
          has_sync = true;
        }
        break;
      }
      case spv::Op::OpAtomicCompareExchange:
      case spv::Op::OpAtomicCompareExchangeWeak:
        if (IsSyncOnUniform(inst->GetSingleWordInOperand(2)) ||
            IsSyncOnUniform(inst->GetSingleWordInOperand(3))) {
          has_sync = true;
        }
        break;
      default:
        break;
    }
  });
  has_uniform_sync_ = has_sync;
  checked_for_uniform_sync_ = true;
  return has_sync;
}

bool CodeSinkingPass::IsSyncOnUniform(uint32_t mem_semantics_id) const {
  const analysis::Constant* mem_semantics_const =
      context()->get_constant_mgr()->FindDeclaredConstant(mem_semantics_id);
  assert(mem_semantics_const != nullptr &&
         "Expecting memory semantics id to be a constant.");
  assert(mem_semantics_const->AsIntConstant() &&
         "Memory semantics should be an integer.");
  uint32_t mem_semantics_int = mem_semantics_const->GetU32();



  if ((mem_semantics_int & uint32_t(spv::MemorySemanticsMask::UniformMemory)) ==
      0) {
    return false;
  }



  return (mem_semantics_int &
          uint32_t(spv::MemorySemanticsMask::Acquire |
                   spv::MemorySemanticsMask::AcquireRelease |
                   spv::MemorySemanticsMask::Release)) != 0;
}

bool CodeSinkingPass::HasPossibleStore(Instruction* var_inst) {
  assert(var_inst->opcode() == spv::Op::OpVariable ||
         var_inst->opcode() == spv::Op::OpAccessChain ||
         var_inst->opcode() == spv::Op::OpPtrAccessChain);

  return get_def_use_mgr()->WhileEachUser(var_inst, [this](Instruction* use) {
    switch (use->opcode()) {
      case spv::Op::OpStore:
        return true;
      case spv::Op::OpAccessChain:
      case spv::Op::OpPtrAccessChain:
        return HasPossibleStore(use);
      default:
        return false;
    }
  });
}

bool CodeSinkingPass::IntersectsPath(uint32_t start, uint32_t end,
                                     const std::unordered_set<uint32_t>& set) {
  std::vector<uint32_t> worklist;
  worklist.push_back(start);
  std::unordered_set<uint32_t> already_done;
  already_done.insert(start);

  while (!worklist.empty()) {
    BasicBlock* bb = context()->get_instr_block(worklist.back());
    worklist.pop_back();

    if (bb->id() == end) {
      continue;
    }

    if (set.count(bb->id())) {
      return true;
    }

    bb->ForEachSuccessorLabel([&already_done, &worklist](uint32_t* succ_bb_id) {
      if (already_done.insert(*succ_bb_id).second) {
        worklist.push_back(*succ_bb_id);
      }
    });
  }
  return false;
}



}
}
